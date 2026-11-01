#include <ale/ale_interface.hpp>
#include <iostream>
#include <cstdlib> 

int main()
{
    ale::ALEInterface ale;

    ale.setInt("random_seed", 42);

    ale.setBool("display_screen", true);

    ale.loadROM("pong.bin");

    ale::ActionVect legal_actions = ale.getLegalActionSet();
    std::cout << "Successfully loaded Atari game!\n";
    std::cout << "Available distinct actions: " << legal_actions.size() << "\n";

    int frames = 0;
    while (!ale.game_over() && frames < 5000) 
    { 

        ale::Action action = legal_actions[rand() % legal_actions.size()];

        float reward = ale.act(action);
        frames++;

        if (reward != 0) 
        {
            std::cout << "Frame " << frames << " | Agent got a reward of: " << reward << "\n";
        }
    }

    std::cout << "Episode complete after " << frames << " frames.\n";
    return 0;
}
