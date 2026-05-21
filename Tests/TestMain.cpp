#include <JuceHeader.h>

int main()
{
    // Initialise the MessageManager so that juce::Timer works in tests
    juce::MessageManager::getInstance();

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult(i)->failures;

    if (failures > 0)
        std::cerr << failures << " test(s) failed.\n";
    else
        std::cout << "All tests passed.\n";

    juce::MessageManager::deleteInstance();
    return failures > 0 ? 1 : 0;
}
