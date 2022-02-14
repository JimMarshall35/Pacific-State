#pragma once
#pragma once
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <map>
#include <assert.h>     /* assert */
#include <mutex>
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <type_traits>

namespace PS {
    
    enum class PSFiringMode {
        Immediate,
        Queued
    };
    template <typename...> class StateMachine;

    template <typename TState, typename TTrigger>
    class StateMachine<TState, TTrigger>
    {
        static_assert(std::is_enum<TState>::value == true, "state machine state type must be an enum");
        static_assert(std::is_enum<TTrigger>::value == true, "state machine trigger type must be an enum");
        static_assert(std::is_same_v<std::underlying_type<TState>::type, unsigned int >, "state machine state type must be based on unsigned int");
        static_assert(std::is_same_v<std::underlying_type<TTrigger>::type, unsigned int >, "state machine trigger type must based mon unsigned int");
    public:
        
#pragma region  internal types

#pragma region  TriggerNotFoundException

        struct TriggerNotFoundException : public std::exception {
            TriggerNotFoundException(TState stateOn, TTrigger trigger)
                :m_state(stateOn), m_trigger(trigger) {}
            const char* what() const throw () {
                std::cout << "[STATE MACHINE EXCEPTION] Trigger " << (unsigned int)m_trigger << " not found on state " << (unsigned int)m_state << std::endl;
                return "trigger not found";
            }
        private:
            TState m_state;
            TTrigger m_trigger;
        };

#pragma endregion
        
#pragma region TransitionInfo

        struct TransitionInfo {
            TState From;
            TState To;
            bool IsReentry = false;
        };

#pragma endregion

#pragma region StateRepresentation

        class StateRepresentation{
        public:
            friend class StateMachine;
            std::vector<std::function<void(TransitionInfo)>> InternalTransitions;
            std::vector<TState> AllowedTransitions;
            StateRepresentation(TState s, StateMachine* sm) :State(s), m_stateMachinePtr(sm) {}
            StateRepresentation() {}
            TState State;
            std::function<void(TransitionInfo)> OnEnter = nullptr;
            std::function<void(TransitionInfo)> OnExit = nullptr;
            void AddTransition(TTrigger trigger, TState state) { 
                AllowedTransitions[(unsigned int)trigger] = state;
            }
            bool FindTransition(TState& transitionOnOutput, TTrigger trigger);
            bool FindInternalTransition(std::function<void(TransitionInfo)>& transitionFunction, TTrigger trigger);
#
            StateRepresentation* SuperState = nullptr;

            std::vector<StateRepresentation*> SubStates;
            
            bool Includes(TState state); // is state this state or one of its sub states
            bool IsIncludedIn(TState state); // Checks if the state is in the set of this state or a super-state
            void Enter(TransitionInfo t);
            void Exit(TransitionInfo t);
        private:
            StateMachine* m_stateMachinePtr = nullptr;
        };

#pragma endregion

#pragma region StateConfigObject

        struct StateConfigObject {
        private:
            StateMachine* m_stateMachinePtr;
        public:
            StateConfigObject(StateMachine* parent) : m_stateMachinePtr(parent){}
            TState StateEnum;
            StateConfigObject Permit(TTrigger trigger, TState state) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.AddTransition(trigger,state);
                return *this;
            }
            StateConfigObject OnEntry(std::function<void(TransitionInfo)> func) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.OnEnter = func;
                return *this;
            }
            StateConfigObject OnExit(std::function<void(TransitionInfo)> func) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.OnExit = func;
                return *this;
            }
            StateConfigObject SubStateOf(TState superstate) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.SuperState = &m_stateMachinePtr->m_states[(int)superstate];
                auto& superstaterep = m_stateMachinePtr->m_states[(int)superstate];
                superstaterep.SubStates.push_back(&staterep);
                return *this;
            }
            StateConfigObject InternalTransition(TTrigger trigger, std::function<void(TransitionInfo)> action) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.InternalTransitions[trigger] = action;
                return *this;
            }
        };
#pragma endregion

#pragma endregion

    public:
        StateConfigObject ConfigState(TState state) {
            StateConfigObject config(this);
            config.StateEnum = state;
            m_states[(int)state].State = state;
            return config;
        }

        StateMachine(int numstates, int numtriggers, TState initialState) 
            : m_currentState(initialState), m_numStates(numstates), m_numTriggers(numtriggers){ 
            m_states.resize(numstates); 
            for (int i = 0; i < numstates; i++) {
                auto& state = m_states[i];
                state.AllowedTransitions.resize(numtriggers);
                state.InternalTransitions.resize(numtriggers);
                state.m_stateMachinePtr = this;
                //memset(state.m_allowedTransitions.data(),)
            }
        }

        ~StateMachine();
        void RunAsync();
        void Fire(TTrigger trigger);
        void TryFire(TTrigger trigger); 
        void FireAsync(TTrigger trigger);
        void HandleEventQueue();
    private:
        void FireInternalImmediate(TTrigger trigger);
        void FireInternalQueued(TTrigger trigger);
        
    private:
        int m_numStates = 0;
        int m_numTriggers = 0;
        TState m_currentState;
        std::vector<StateRepresentation> m_states;
        std::queue<TTrigger> m_eventQueue;
        PSFiringMode m_firingMode = PSFiringMode::Queued;
        std::mutex m_eventQueueMutex;
        std::mutex m_firingMutex;
        std::atomic_bool m_asyncMode = false;
        std::unique_ptr<std::thread> m_asyncThread;
    };

#pragma region StateRepresentation
    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::FindInternalTransition(std::function<void(TransitionInfo)>& transitionFunction, TTrigger trigger)
    {
        auto& currentState = m_stateMachinePtr->m_states[(unsigned int)State];
        auto& v = currentState.InternalTransitions;

        if (v[(int)trigger] != nullptr) {
            transitionFunction = v[(int)trigger];
            return true;
        }
        else if (SuperState) {
            return SuperState->FindInternalTransition(transitionFunction, trigger);
        }
        return false;
    }


    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::FindTransition(TState& transitionOnOutput, TTrigger trigger)
    {
        auto& currentState = m_stateMachinePtr->m_states[(unsigned int)State];
        auto& v = currentState.AllowedTransitions;

        if ((int)v[(int)trigger]) {
            transitionOnOutput = v[(int)trigger];
            return true;
        }
        else if (SuperState) {
            auto tr = SuperState->FindTransition(transitionOnOutput, trigger);
            return tr;
        }
        return false;
    }

    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::Includes(TState state)
    {

        if (state == State)
            return true;
        for (auto s : SubStates) {
            if (s->State == state)
                return true;
        }
        return false;
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::StateRepresentation::Enter(TransitionInfo t)
    {
        if (t.IsReentry) {
            OnEnter(t);
        }
        else if (!Includes(t.From)) {
            if (SuperState)
                SuperState->Enter(t);
            OnEnter(t);
        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::StateRepresentation::Exit(TransitionInfo t)
    {
        if (t.IsReentry) {
            OnExit(t);
        }
        else if (!Includes(t.To)) {
            OnExit(t);
            if (SuperState != nullptr) {
                if (IsIncludedIn(t.To)) {

                }
                else {
                    SuperState->Exit(t);
                }
            }
        }
    }

    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::IsIncludedIn(TState state)
    {
        return State == state || (SuperState != nullptr && SuperState->IsIncludedIn(state));
    }

#pragma endregion

#pragma region StateMachine


    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::Fire(TTrigger trigger)
    {
        if (m_firingMode == PSFiringMode::Immediate) {
            FireInternalImmediate(trigger);
        }
        else if (m_firingMode == PSFiringMode::Queued) {
            FireInternalQueued(trigger);
        }
        

    }


    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::TryFire(TTrigger trigger)
    {
        try {
            Fire(trigger);
        }
        catch(std::exception& e){
            
        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::FireInternalImmediate(TTrigger trigger)
    {
        auto& currentState = m_states[(unsigned int)m_currentState];
        auto& internalTransitions = currentState.InternalTransitions;

        
        // see if there is a transition to another state allowed
        auto& v = currentState.AllowedTransitions;
        TState nextstateEnum;
        if (!currentState.FindTransition(nextstateEnum, trigger)) {
            // if not check for any internal transtitions
            TransitionInfo t;
            t.From = m_currentState;
            t.To = m_currentState;
            std::function<void(TransitionInfo)> internalT = nullptr;
            if (currentState.FindInternalTransition(internalT, trigger)) {
                // if an internal transiton has been found, do the action
                // and then return
                internalT(t);
                return;
            }
            else {
                // if not throw an exception, trigger not found
                // on external or internal transitions
                throw TriggerNotFoundException(currentState.State, trigger);
            }
            
        }
        // if this point has been reached, there's an external transition
        // for this trigger. Do entry and exit actions
        TransitionInfo t;
        t.From = m_currentState;
        t.To = nextstateEnum;
        bool onExitException = false;
        if (currentState.OnExit) {
            try {
                currentState.Exit(t);
            }
            catch (std::exception e) {

                onExitException = true;
            }

        }
        auto& nextstate = m_states[(unsigned int)nextstateEnum];
        bool onEnterException = false;
        if (nextstate.OnEnter) {
            try {
                nextstate.Enter(t);
            }
            catch (std::exception e) {
                onEnterException = true;
            }
        }
        m_currentState = nextstateEnum;

        if (onExitException) {
            //std::cout << 
            // fire state machine faulted event
        }
        if (onEnterException) {
            // fire state machine faulted event
        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::FireInternalQueued(TTrigger trigger)
    {
        static bool firing = false;

        {
            std::lock_guard<std::mutex> lg(m_eventQueueMutex);
            m_eventQueue.push(trigger);
        }
        {
            std::lock_guard<std::mutex> lg(m_firingMutex);
            if (firing) {
                return;
            }
            else {
                firing = true;
            }
        }
        
        try {                         // microsoft 
            
            while (!m_eventQueue.empty()) {
                TTrigger trigger = m_eventQueue.front();
                m_eventQueue.pop();
                FireInternalImmediate(trigger);
            }
        }
        catch (TriggerNotFoundException e) {
            std::string txt = e.what();
            std::cout << txt << std::endl;
            firing = false;
            throw e;
        }
        catch(std::exception& e) {
            std::string txt = e.what();
            std::cout << txt << std::endl;
            firing = false;
            throw e;
        }
        
        firing = false;
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::RunAsync()
    {
        auto worker = [this]() {
            while (m_asyncMode) {
                HandleEventQueue();
            }
        };
        m_asyncMode = true;
        m_asyncThread = std::make_unique<std::thread>(std::thread(worker));
    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::FireAsync(TTrigger trigger)
    {
        {
            std::lock_guard<std::mutex> lg(m_eventQueueMutex);
            m_eventQueue.push(trigger);
        }
    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::HandleEventQueue()
    {
        while (!m_eventQueue.empty()) {
            TTrigger trigger = m_eventQueue.front();
            m_eventQueue.pop();
            try {
                FireInternalImmediate(trigger);
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }
    }

    template<typename TState, typename TTrigger>
    inline StateMachine<TState, TTrigger>::~StateMachine()
    {
        if (m_asyncMode == true) {
            m_asyncMode = false;
            m_asyncThread->join();
        }
    }

#pragma endregion

} // end of namespace PS
