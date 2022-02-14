#include "PacificState.h"
#include <iostream>
#include <thread>
#define THREAD_SLEEP_MS(ms) std::this_thread::sleep_for(std::chrono::duration<double,std::milli>(ms));

enum class States : unsigned int {
	None = 0,
	Stopped,
	Loaded,
	Running,
	Paused
};
enum class Triggers : unsigned int {
	None = 0,
	Play,
	Pause,
	Stop
};

std::string statenames[] = { "None", "Stopped", "Loaded","Running", "Paused"};
std::string triggerNames[] = { "None", "Play", "Pause","Stop" };





int main(int argc, char* argv[]) { 
	std::cout << "fojdsiofjdsoi world" << std::endl;
	PS::StateMachine<States, Triggers> stateMachine(5,4, States::Stopped);

	stateMachine.ConfigState(States::Stopped)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "entering stopped" << std::endl;
		})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "exiting stopped" << std::endl;
		})
		.Permit(Triggers::Play, States::Running);

	stateMachine.ConfigState(States::Running)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "entering Running" << std::endl;
		})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "exiting Running" << std::endl;
		})
		.Permit(Triggers::Pause, States::Paused)
		.SubStateOf(States::Loaded);
	
	stateMachine.ConfigState(States::Paused)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "entering Paused" << std::endl;
		})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "exiting Paused" << std::endl;
		})
		.Permit(Triggers::Play, States::Running)
		.SubStateOf(States::Loaded);
		
	stateMachine.ConfigState(States::Loaded)
		.OnEntry([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "entering loaded" << std::endl;
		})
		.OnExit([](PS::StateMachine< States, Triggers>::TransitionInfo t) {
			std::cout << "exiting loaded" << std::endl;
		})
		.Permit(Triggers::Stop, States::Stopped);
	stateMachine.RunAsync();
	stateMachine.FireAsync(Triggers::Play);
	stateMachine.FireAsync(Triggers::Pause);
	stateMachine.FireAsync(Triggers::Stop);
	stateMachine.FireAsync(Triggers::Play);
	stateMachine.FireAsync(Triggers::Stop);
	stateMachine.FireAsync(Triggers::Play);
	stateMachine.FireAsync(Triggers::Pause);
	stateMachine.FireAsync(Triggers::Pause);

	/*
	stateMachine.Fire(Triggers::Play);
	stateMachine.Fire(Triggers::Pause);
	stateMachine.Fire(Triggers::Stop);
	stateMachine.Fire(Triggers::Play);
	stateMachine.Fire(Triggers::Stop);
	stateMachine.Fire(Triggers::Play);
	stateMachine.Fire(Triggers::Pause);
	stateMachine.TryFire(Triggers::Pause);
	*/
	THREAD_SLEEP_MS(3000);
}