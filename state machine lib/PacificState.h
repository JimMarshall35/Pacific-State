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

        class StateRepresentation {
        public:
            StateRepresentation(TState s, StateMachine* sm) :State(s), m_stateMachinePtr(sm) {}
            StateRepresentation() {}
            // setters
            void AddTransition(TTrigger trigger, TState state);
            void AddInternalTransition(TTrigger trigger, std::function<void(TransitionInfo)> action);
            void AddSubState(StateRepresentation* subState) { m_subStates.push_back(subState); }
            void SetSuperState(StateRepresentation* superState) { m_superState = superState; }
            void SetOnEnter(std::function<void(TransitionInfo)> handler);
            void SetOnExit(std::function<void(TransitionInfo)> handler);
            void AddGuardClause(TTrigger trigger, std::function<bool()> guard);
            
            bool FindTransition(TState& transitionOnOutput, TTrigger trigger);
            bool FindInternalTransition(std::function<void(TransitionInfo)>& transitionFunction, TTrigger trigger);
            bool Includes(TState state); // is state this state or one of its sub states
            bool IsIncludedIn(TState state); // Checks if the state is in the set of this state or a super-state
            void Enter(TransitionInfo t);
            void Exit(TransitionInfo t);
            bool HasEnterHandler() { return m_onEnter != nullptr; }
            bool HasExitHandler() { return m_onExit != nullptr; }
            void ResizeTransitions(size_t val) { m_allowedTransitions.resize(val); }
            void ResizeInternalTransitions(size_t val) { m_internalTransitions.resize(val); }
            void ResizeGuardClauses(size_t val) { m_guardClauses.resize(val); }
            void SetStateMachinePtr(StateMachine* ptr) { m_stateMachinePtr = ptr; }
            void GetAllowedTransitions(std::vector<TTrigger>& returnvec) const;
            const std::vector<std::function<bool()>>& GetGuardClauses() { return m_guardClauses; }

        public:
            TState State;

        private:
            std::vector<std::function<bool()>> m_guardClauses;
            StateMachine* m_stateMachinePtr = nullptr;
            std::vector<TState> m_allowedTransitions;
            std::vector<std::function<void(TransitionInfo)>> m_internalTransitions;
            

            std::function<void(TransitionInfo)> m_onEnter = nullptr;
            std::function<void(TransitionInfo)> m_onExit = nullptr;
            StateRepresentation* m_superState = nullptr;
            std::vector<StateRepresentation*> m_subStates;

            
            
        };

#pragma endregion

#pragma region StateConfigObject

        struct StateConfigObject {
        private:
            StateMachine* m_stateMachinePtr;
        public:
            StateConfigObject(StateMachine* parent) : m_stateMachinePtr(parent) {}
            TState StateEnum;
            StateConfigObject Permit(TTrigger trigger, TState state) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.AddTransition(trigger, state);
                return *this;
            }
            StateConfigObject OnEntry(std::function<void(TransitionInfo)> func) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.SetOnEnter(func);
                return *this;
            }
            StateConfigObject OnExit(std::function<void(TransitionInfo)> func) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.SetOnExit(func);
                return *this;
            }
            StateConfigObject SubStateOf(TState superstate) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.SetSuperState(&m_stateMachinePtr->m_states[(int)superstate]);
                auto& superstaterep = m_stateMachinePtr->m_states[(int)superstate];
                superstaterep.AddSubState(&staterep);
                return *this;
            }
            StateConfigObject InternalTransition(TTrigger trigger, std::function<void(TransitionInfo)> action) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.AddInternalTransition(trigger, action);
                return *this;
            }
            StateConfigObject PermitIf(TTrigger trigger, TState state, std::function<bool()> guard) {
                auto& staterep = m_stateMachinePtr->m_states[(int)StateEnum];
                staterep.AddTransition(trigger, state);
                staterep.AddGuardClause(trigger, guard);
                return *this;
            }
        };
#pragma endregion

#pragma region TriggerEventData

        enum class TriggerEventDataTypes { Int, Bool, String, Float, Double, CharPtr };
        struct TriggerEventData
        {
            TriggerEventDataTypes Tag;
            union Value {
                int i;
                bool b;
                std::string s;
                float f;
                double d;
                char* cp;
            }val;
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
        StateMachine(int numstates, int numtriggers, TState initialState);
        ~StateMachine();
        void RunActive();
        void Fire(TTrigger trigger);
        void TryFire(TTrigger trigger);
        void FireAsync(TTrigger trigger);
        void HandleEventQueue();
        std::vector<TTrigger> GetCurrentAvailableTransitions() const;
        inline bool GetIsFiringEvents() const { return m_isFiringEvents; }
        bool EventQueueEmpty();
    private:
        void FireInternalImmediate(TTrigger trigger);
        void FireInternalQueued(TTrigger trigger);

    private:
        int m_numStates = 0;
        int m_numTriggers = 0;
        std::atomic_bool m_isFiringEvents = false;
        TState m_currentState;
        std::vector<StateRepresentation> m_states;
        std::queue<TTrigger> m_eventQueue;
        PSFiringMode m_firingMode = PSFiringMode::Queued;
        std::mutex m_eventQueueMutex;


        std::atomic_bool m_asyncMode = false;
        std::unique_ptr<std::thread> m_asyncThread;
    };

#pragma region StateRepresentation
    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::FindInternalTransition(std::function<void(TransitionInfo)>& transitionFunction, TTrigger trigger)
    {
        if (m_internalTransitions[(int)trigger] != nullptr) {
            transitionFunction = m_internalTransitions[(int)trigger];
            return true;
        }
        else if (m_superState) {
            return m_superState->FindInternalTransition(transitionFunction, trigger);
        }
        return false;
    }


    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::AddTransition(TTrigger trigger, TState state)
    {
        m_allowedTransitions[(unsigned int)trigger] = state;

    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::AddInternalTransition(TTrigger trigger, std::function<void(TransitionInfo)> action)
    {
        m_internalTransitions[(unsigned int)trigger] = action;
    }





    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::SetOnEnter(std::function<void(TransitionInfo)> handler)
    {
        m_onEnter = handler;
    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::SetOnExit(std::function<void(TransitionInfo)> handler)
    {
        m_onExit = handler;
    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::AddGuardClause(TTrigger trigger, std::function<bool()> guard) 
    {
        m_guardClauses[(unsigned int)trigger] = guard;
    }

    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::FindTransition(TState& transitionOnOutput, TTrigger trigger)
    {

        if ((int)m_allowedTransitions[(int)trigger]) {
            transitionOnOutput = m_allowedTransitions[(int)trigger];
            return true;
        }
        else if (m_superState) {
            auto tr = m_superState->FindTransition(transitionOnOutput, trigger);
            return tr;
        }
        return false;
    }

    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::Includes(TState state)
    {

        if (state == State)
            return true;
        for (auto s : m_subStates) {
            if (s->State == state)
                return true;
        }
        return false;
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::StateRepresentation::Enter(TransitionInfo t)
    {
        if (t.IsReentry) {
            m_onEnter(t);
        }
        else if (!Includes(t.From)) {
            if (m_superState)
                m_superState->Enter(t);
            m_onEnter(t);
        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::StateRepresentation::Exit(TransitionInfo t)
    {
        if (t.IsReentry) {
            m_onExit(t);
        }
        else if (!Includes(t.To)) {
            m_onExit(t);
            if (m_superState != nullptr) {
                if (IsIncludedIn(t.To)) {

                }
                else {
                    m_superState->Exit(t);
                }
            }
        }
    }

    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::StateRepresentation::GetAllowedTransitions(std::vector<TTrigger>& returnvec) const
    {

        for (int i = 0; i < m_allowedTransitions.size(); i++) {
            if ((int)m_allowedTransitions[i] != 0) {
                returnvec.push_back((TTrigger)i);
            }
        }
        for (int i = 0; i < m_internalTransitions.size(); i++) {
            if (m_internalTransitions[i] != nullptr) {
                returnvec.push_back((TTrigger)i);
            }
        }
    }


    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::StateRepresentation::IsIncludedIn(TState state)
    {
        return State == state || (m_superState != nullptr && m_superState->IsIncludedIn(state));
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
        catch (std::exception& e) {

        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::FireInternalImmediate(TTrigger trigger)
    {
        auto& currentState = m_states[(unsigned int)m_currentState];
        const auto& guardClauses = currentState.GetGuardClauses();
        /*
            if there is an internal transition set up this will override the same trigger being set as an external transition

            could change this to be that the internal transition will fire and then the external trigger, don't know if this would be preferable or not
        */


        TState nextstateEnum;

        // see if there is any external transitions allowed.
        // if not check for any internal transtitions
        if (!currentState.FindTransition(nextstateEnum, trigger)) {
            TransitionInfo t;
            t.From = m_currentState;
            t.To = m_currentState;
            std::function<void(TransitionInfo)> internalT = nullptr;
            if (currentState.FindInternalTransition(internalT, trigger)) {
                if (guardClauses[(int)trigger] != nullptr) {
                    if (!guardClauses[(int)trigger]()) {
                        std::cout << "guard clause failed " << std::endl;
                        return;
                    }
                }
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
        if (guardClauses[(int)trigger] != nullptr) {
            if (!guardClauses[(int)trigger]()) {
                std::cout << "guard clause failed " << std::endl;
                return;
            }
        }
        TransitionInfo t;
        t.From = m_currentState;
        t.To = nextstateEnum;
        bool onExitException = false;
        if (currentState.HasExitHandler()) {
            try {
                currentState.Exit(t);
            }
            catch (std::exception e) {

                onExitException = true;
            }

        }
        auto& nextstate = m_states[(unsigned int)nextstateEnum];
        bool onEnterException = false;
        if (nextstate.HasEnterHandler()) {
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
        // will queue the trigger and then process triggers on the queue until it is empty.
        // if another thread calls this method while the 
        static bool s_isQueueBeingHandled = false;
        static std::mutex s_isQueueBeingHandledMutex;

        {
            std::lock_guard<std::mutex> lg(m_eventQueueMutex);
            m_eventQueue.push(trigger);
        }
        {
            std::lock_guard<std::mutex> lg(s_isQueueBeingHandledMutex);
            if (s_isQueueBeingHandled) {
                return;
            }
            else {
                s_isQueueBeingHandled = true;
            }
        }

        try {
            while (!EventQueueEmpty()) {
                TTrigger trigger = m_eventQueue.front();
                m_eventQueue.pop();
                FireInternalImmediate(trigger);
            }
        }
        catch (TriggerNotFoundException e) {
            std::string txt = e.what();
            std::cout << txt << std::endl;
            s_isQueueBeingHandled = false;
            throw e;
        }
        catch (std::exception& e) {
            std::string txt = e.what();
            std::cout << txt << std::endl;
            s_isQueueBeingHandled = false;
            throw e;
        }

        {
            std::lock_guard<std::mutex> lg(s_isQueueBeingHandledMutex);
            s_isQueueBeingHandled = false;
        }
    }

    template<typename TState, typename TTrigger>
    inline void PS::StateMachine<TState, TTrigger>::RunActive()
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
            m_isFiringEvents = true;
        }
    }


    template<typename TState, typename TTrigger>
    inline void StateMachine<TState, TTrigger>::HandleEventQueue()
    {

        while (!EventQueueEmpty()) {
            TTrigger trigger = m_eventQueue.front();
            m_eventQueue.pop();
            try {
                FireInternalImmediate(trigger);
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }
        if (m_isFiringEvents)
            m_isFiringEvents = false;
    }



    template<typename TState, typename TTrigger>
    inline StateMachine<TState, TTrigger>::StateMachine(int numstates, int numtriggers, TState initialState)
        : m_currentState(initialState), m_numStates(numstates), m_numTriggers(numtriggers) {
        m_states.resize(numstates);
        for (int i = 0; i < numstates; i++) {
            auto& state = m_states[i];
            state.ResizeTransitions(numtriggers);
            state.ResizeInternalTransitions(numtriggers);
            state.ResizeGuardClauses(numtriggers);
            state.SetStateMachinePtr(this);
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

    template<typename TState, typename TTrigger>
    inline std::vector<TTrigger> PS::StateMachine<TState, TTrigger>::GetCurrentAvailableTransitions() const
    {
        std::vector<TTrigger> returnvec;
        m_states[(int)m_currentState].GetAllowedTransitions(returnvec);
        return returnvec;
    }

    template<typename TState, typename TTrigger>
    inline bool PS::StateMachine<TState, TTrigger>::EventQueueEmpty()
    {
        bool returnval;

        {
            std::lock_guard<std::mutex> lg(m_eventQueueMutex);
            returnval = m_eventQueue.empty();
        }
        return returnval;
    }

#pragma endregion


} // end of namespace PS
