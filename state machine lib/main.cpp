#include "PacificState.h"
#include <iostream>
#include <thread>
#define THREAD_SLEEP_MS(ms) std::this_thread::sleep_for(std::chrono::duration<double,std::milli>(ms));

#define TYPE3

enum class States : unsigned int {
	None = 0,
	LoadingInitial = 1,
	Intro = 2,
	MainMenu = 3,
	Playing = 4,
	PausedMenu = 5,
	EditingLevel = 6,
	NonPlaying = 7,
	LoadingProgress = 8,
	SavingProgress = 9
};
enum class Triggers : unsigned int {
	None = 0,
	Skip = 1,
	Finish = 2,
	Play = 3,
	Edit = 4,
	Pause = 5,
	QuitToMainMenu = 6,
	Load = 7,
	Save = 8,
	ToggleMute = 9
};

std::string statenames[] = { "None", "Loading", "Intro","MainMenu", "Playing","PausedMenu","EditingLevel", "NonPlaying","LoadingProgress", "SavingProgress"};
std::string triggerNames[] = { "None", "Skip", "Finish","Play" , "Edit", "Pause", "QuitToMainMenu","Load","Save","ToggleMute"};


void PrintPossibleTriggers(std::vector<Triggers>& allowedTriggers);
void AwaitEventHandling(const PS::StateMachine<States,Triggers>& sm);

int main(int argc, char* argv[]) { 
	static bool muted = false;
	std::cout << "fojdsiofjdsoi world" << std::endl;
	PS::StateMachine<States, Triggers> stateMachine(9,10, States::LoadingInitial);
	std::cout << "entering loading" << std::endl;
	stateMachine.ConfigState(States::LoadingInitial)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "entering loading" << std::endl;
		})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "exiting loading" << std::endl;
		})
		.Permit(Triggers::Finish, States::Intro);

	stateMachine.ConfigState(States::Intro)
		.SubStateOf(States::NonPlaying)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering intro" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting intro" << std::endl;
			})
		.Permit(Triggers::Skip, States::MainMenu)
		.Permit(Triggers::Finish, States::MainMenu);

	stateMachine.ConfigState(States::MainMenu)
		.SubStateOf(States::NonPlaying)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering main menu" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting main menu" << std::endl;
			})
		.Permit(Triggers::Play, States::Playing);
	
	stateMachine.ConfigState(States::Playing)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering playing" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting playing" << std::endl;
			})
		.Permit(Triggers::Pause, States::PausedMenu)
		.Permit(Triggers::Edit, States::EditingLevel)
		.Permit(Triggers::QuitToMainMenu, States::MainMenu);

	stateMachine.ConfigState(States::PausedMenu)
		.SubStateOf(States::NonPlaying)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering paused menu" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting paused menu" << std::endl;
			})
		.Permit(Triggers::Play, States::Playing)
		.PermitIf(Triggers::QuitToMainMenu, States::MainMenu, []() {
				if (!muted) {
					std::cout << "You're audio is not muted and so you can't quit to the main menu - nice feature" << std::endl;
				}
				return muted;
			})
		.InternalTransition(Triggers::ToggleMute, [](PS::StateMachine< States, Triggers>::TransitionInfo info) {
				
				if (muted) {
					muted = false;
				}
				else{
					muted = true;
				}
				std::cout << (muted ? "muted" : "unmuted") << std::endl;
			});

	stateMachine.ConfigState(States::EditingLevel)
		.SubStateOf(States::NonPlaying)
		.OnEntry([](PS::StateMachine<States, Triggers>::TransitionInfo t) {
				std::cout << "entering level editor" << std::endl;
				throw std::exception();
			})
		.OnExit([](PS::StateMachine<States, Triggers>::TransitionInfo t) {
				std::cout << "exiting level editor" << std::endl;
			})
		.Permit(Triggers::Play, States::Playing);


	stateMachine.ConfigState(States::NonPlaying)
		.OnEntry([](PS::StateMachine<States, Triggers>::TransitionInfo t) {
				std::cout << "entering NonPlaying" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting NonPlaying" << std::endl;
			});


	


#ifdef TYPE1
	/*
		normal fire mode
	*/
	while (true) {
		std::vector<Triggers> allowedTriggers = stateMachine.GetCurrentAllowedTransitions();
		PrintPossibleTriggers(allowedTriggers);
		int input;
		std::cin >> input;
		if (input >= 0 && input < allowedTriggers.size()) {
			std::cout << "firing " << triggerNames[(int)allowedTriggers[input]] << std::endl;
			stateMachine.Fire(allowedTriggers[input]);
		}
		else {
			std::cout << "transition not recognised" << std::endl;
		}
	}

	
#endif
#ifdef TYPE2
	/*
		HandleEventQueue can be called where you choose, queue events in a thread safe way with FireAsync
	*/
	while (true) {
		std::vector<Triggers> allowedTriggers = stateMachine.GetCurrentAllowedTransitions();
		PrintPossibleTriggers(allowedTriggers);
		int input;
		std::cin >> input;
		if (input >= 0 && input < allowedTriggers.size()) {
			std::cout << "firing " << triggerNames[(int)allowedTriggers[input]] << std::endl;
			stateMachine.FireAsync(allowedTriggers[input]);
		}
		else {
			std::cout << "transition not recognised" << std::endl;
		}
		stateMachine.HandleEventQueue();
	}
#endif
#ifdef TYPE3
	/*
		HandleEventQueue is called in a loop on a separate thread.
		RunActive() can be used to spawn a worker thread that does this that's torn down with the state machine.
	*/
	stateMachine.RunActive();
	while (true) {
		// wait for the event to finish being handled (if one is being)
		// so that the allowed transitions we read back will be from the new state
		AwaitEventHandling(stateMachine);
		// get allowed transitions and print the choices
		std::vector<Triggers> allowedTriggers = stateMachine.GetCurrentAvailableTransitions();
		PrintPossibleTriggers(allowedTriggers);
		// wait for a choice to be inputted
		int input;
		std::cin >> input;
		// if the input is valid, fire the users choice of trigger
		if (input >= 0 && input < allowedTriggers.size()) {
			std::cout << "firing " << triggerNames[(int)allowedTriggers[input]] << std::endl;
			stateMachine.FireAsync(allowedTriggers[input]);
		}
		else {
			std::cout << "transition not recognised" << std::endl;
		}
	}
#endif


	THREAD_SLEEP_MS(3000);
}

void PrintPossibleTriggers(std::vector<Triggers>& allowedTriggers)
{
	std::cout << "\n\n" << "Choose a transition: " << std::endl;
	for (int i = 0; i < allowedTriggers.size(); i++) {
		auto trigger = (int)allowedTriggers[i];
		std::cout << i << ".) " << triggerNames[trigger] << std::endl;
	}
}

void AwaitEventHandling(const PS::StateMachine<States, Triggers>& sm)
{
	while (sm.GetIsFiringEvents()) {}
}
