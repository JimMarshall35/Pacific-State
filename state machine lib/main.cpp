#include "PacificState.h"
#include <iostream>
#include <thread>
#define THREAD_SLEEP_MS(ms) std::this_thread::sleep_for(std::chrono::duration<double,std::milli>(ms));

enum class States : unsigned int {
	None = 0,
	Loading = 1,
	Intro = 2,
	MainMenu = 3,
	Playing = 4,
	PausedMenu = 5,
	EditingLevel = 6,
	NonPlaying = 7
};
enum class Triggers : unsigned int {
	None = 0,
	Skip,
	Finish,
	Play,
	Edit,
	Pause,
	QuitToMainMenu
};

std::string statenames[] = { "None", "Loading", "Intro","MainMenu", "Playing","PausedMenu","EditingLevel"};
std::string triggerNames[] = { "None", "Skip", "Finish","Play" , "Edit", "Pause", "QuitToMainMenu"};





int main(int argc, char* argv[]) { 
	std::cout << "fojdsiofjdsoi world" << std::endl;
	PS::StateMachine<States, Triggers> stateMachine(8,7, States::Loading);
	std::cout << "entering loading" << std::endl;
	stateMachine.ConfigState(States::Loading)
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
		.Permit(Triggers::QuitToMainMenu, States::MainMenu);

	stateMachine.ConfigState(States::EditingLevel)
		.SubStateOf(States::NonPlaying)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering level editor" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting level editor" << std::endl;
			})
		.Permit(Triggers::Play, States::Playing);


	stateMachine.ConfigState(States::NonPlaying)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "entering NonPlaying" << std::endl;
			})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
				std::cout << "exiting NonPlaying" << std::endl;
			});


	stateMachine.RunAsync();

	while (true) {
		while (stateMachine.GetIsFiringEvents()) {};
		std::vector<Triggers> allowedTriggers = stateMachine.GetCurrentAllowedTransitions();
		std::cout << "\n\n" << "Choose a transition: " << std::endl;
		for (int i = 0; i < allowedTriggers.size(); i++ ) {
			auto trigger = (int)allowedTriggers[i];
			std::cout << i <<".) " << triggerNames[trigger] << std::endl;
		}
		int input;
		std::cin >> input;
		if (input >= 0 && input < allowedTriggers.size()) {
			std::cout << "firing " << triggerNames[(int)allowedTriggers[input]] << std::endl;
			stateMachine.FireAsync(allowedTriggers[input]);
		}
		else {
			std::cout << "transition not recognised" << std::endl;
		}
	}

	THREAD_SLEEP_MS(3000);
}