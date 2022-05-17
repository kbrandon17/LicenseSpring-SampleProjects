#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/Exceptions.h>
#include <iostream>
#include <thread>

//These headers are only necessary for the fibonacci functions below.
#include <string>
#include <time.h>

using namespace LicenseSpring;
int fib( int n );
void fib_game( int max_term );

//Sample code for features licensing. To test feature consumption, our feature will be a fibonacci calculator.
//Using the fibonacci calculator will cost you one consumption. For our feature activation, we will have a 
//fibonacci game. Only the max activation amount of people will be able to use the game at any point.
int main()
{
    
    std::string appName = "NAME"; //input name of application
    std::string appVersion = "VERSION"; //input version of application

    //Collecting network info
    ExtendedOptions options;
    options.collectNetworkInfo( true );
    options.enableLogging( true );

    std::shared_ptr<Configuration> pConfiguration = Configuration::Create(
        EncryptStr( "XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX" ), // your LicenseSpring API key (UUID)
        EncryptStr( "XXXXXXXXX-XXXXX-XXXXXXXXXXXXX_XXXXXX_XXXXXX" ), // your LicenseSpring Shared key
        EncryptStr( "XXXXXX" ), // product code that you specified in LicenseSpring for your application
        appName, appVersion, options);

    //Key-based implementation
    auto licenseId = LicenseID::fromKey( "XXXX-XXXX-XXXX-XXXX" ); //input license key

    //For user-based implementation comment out above line, and use bottom 3 lines
 //   const std::string userId = "example@email.com"; //input user email
 //   const std::string userPassword = "password"; //input user password
 //   auto licenseId = LicenseID::fromUser( userId, userPassword );
    
    std::shared_ptr<LicenseManager> licenseManager = LicenseManager::create( pConfiguration );

    License::ptr_t license = nullptr;

    //Find our local license if we have one stored on our device.
    try
    {
        license = licenseManager->reloadLicense();
    }
    catch ( LocalLicenseException )
    {  //Exception if we cannot read the local license or the local license file is corrupt
        std::cout << "Could not read previous local license. Local license may be corrupt. "
            << "Create a new local license by activating your license." << std::endl;
        return 0;
    }
    
    //If we don't have a local license yet, we'll try activating it first. Otherwise we'll do an online
    //check to sync up our license with the backend. Note, if you recently reset your license, then you may get
    //an exception here. You can fix this by deleting your local license folder located at 
    //C:\Users\%USERPROFILE%\AppData\Local\LicenseSpring\'product code'.
    try
    {
        if ( license == nullptr )
            licenseManager->activateLicense( licenseId );
        else
            license->check();

        //We'll do a local check right after just to make sure everything is working properly.
        license->localCheck();
    }
    //Possible LicenseSpring exceptions, we won't pay too much attention on this tutorial and assume
    //everything is working properly for the user.
    catch ( LicenseSpringException ex )
    {
        std::cout << ex.what() << std::endl;
        return 0;
    }
    
    std::string sInput = "";
    
    while ( sInput.compare( "e" ) != 0 )
    {
        std::cout << "To test our product feature 1: fibonacci calculator, type '1'." << std::endl;
        std::cout << "To test our product feature 2: fibonacci game, type '2'." << std::endl;
        std::cout << "To exit, type 'e'." << std::endl;
        std::cout << ">";
        std::getline( std::cin, sInput );

        //This is our example for our first feature, a fibonacci calculator (see code at bottom for description)
        if ( sInput.compare( "1" ) == 0 )
        {
            try
            {
                license->syncFeatureConsumption( "XXXXXX" ); //Input consumption feature code
                LicenseFeature feature1 = license->feature( "XXXXXX" ); //Input consumption feature code

                //In this case, syncFeatureConsumption will automatically throw an 
                //InvalidLicenseFeatureException if our license is invalid, since, when expired, a feature
                //will delete itself from the platform, so this isExpired() check is a bit redundant. But, 
                //this is a good demonstratation of what you could do for offline cases, and how to check
                //expiry date without using syncFeatureConsumption.
                if (feature1.isExpired())
                {
                    std::cout << "This feature is expired." << std::endl;
                    continue;
                }
                //For this tutorial, we'll be using total consumption instead of local consumption.
                //Total consumption is a consumption pool that all users on the same license shares. Therefore,
                //if another user on the same license uses one consumption, it'll be reflected on all other license
                //users' licenses too. Local consumption is only relative to a device. So each device's consumption will
                //not affect other user's consumption count (unless synced). This can be useful if, for example, 
                //you want all user's to have x amount of consumptions, independent of one another.
                std::cout << "You have a total of: " << feature1.totalConsumption() << " consumptions used so far on this feature." << std::endl;
                
                if ( feature1.totalConsumption() > feature1.maxConsumption() )
                {
                    //This is the case where the user is in the max-overages. You can do something special in this case
                    //, or just notify the user they are in the max-overages.
                    std::cout << "You are currently using max overages." << std::endl;
                    std::cout << "You are currently " << feature1.totalConsumption() - feature1.maxConsumption() <<
                        " consumptions over on this feature." << std::endl;
                    //Note, currently we don't have a maxOverage field for feature consumptions in terms of code.
                }
                else
                {
                    //If the user is not in max overages, you can have your normal code.
                    std::cout << "You have " << feature1.maxConsumption() - feature1.totalConsumption() <<
                        " consumptions left on this feature, before you are in the overage territory." << std::endl;
                }
                std::cout << "Input a fibonacci term to calculate..." << std::endl;
                std::cout << ">";

                //Here we'll implement our feature, which is a fibonacci calculator. 
                std::string fib_string = "";
                std::getline( std::cin, fib_string );
                std::cout << fib( stoi( fib_string ) ) << std::endl;

                //Once the feature succesfully returns, and we don't have an exception, we'll update our consumption 
                //count for this feature. The 'true' field means we save the newly updated consumption count to 
                //our local license file as well.
                license->updateFeatureConsumption( "XXXXXX", 1, true ); //Input consumption feature code
            }
            catch ( NotEnoughConsumptionException ) //This exception is pretty useful to find out when you are out of consumptions
            {
                std::cout << "You are out of consumptions on this feature. " 
                    << "Please reset for more consumptions on this feature." << std::endl;
            }
            catch ( InvalidLicenseFeatureException ) //This exception is used if the license doesn't have the feature added
            {
                std::cout << "Could not find feature. Feature does not exist. "
                    << "Please make sure you inputted the correct feature code and that your feature "
                    << "exists on your license." << std::endl;
            }
            catch ( std::invalid_argument ) //This exception is because we used stoi(string)
            {
                std::cout << "Please input a valid number." << std::endl;
            }
            //Here we'll catch any other exception, although they aren't particularly important to this tutorial
            //so we won't go through all of them.
            catch ( LicenseSpringException ex )
            {
                std::cout << ex.what() << std::endl;
                return 0;
            }
            
        }
        //This is our example for our second feature: a fibonacci game (see code at bottom for description)
        //It is the exact same as the code above, but we use a different feature code, and instead of using our
        //fibonacci calculator at the end, we'll use our game.
        else if ( sInput.compare( "2" ) == 0 ) 
        {
            try
            {
                license->syncFeatureConsumption( "XXXXXX" );
                LicenseFeature feature2 = license->feature( "XXXXXX" );

                if (feature2.isExpired())
                {
                    std::cout << "This feature is expired." << std::endl;
                    continue;
                }
                
                std::cout << "You have a total of: " << feature2.totalConsumption() << " consumptions used so far on this feature." << std::endl;

                if ( feature2.totalConsumption() > feature2.maxConsumption() )
                {
                    //This is the case where the user is in the max-overages. You can do something special in this case
                    //, or just notify the user they are in the max-overages.
                    std::cout << "You are currently using max overages." << std::endl;
                    std::cout << "You are currently " << feature2.totalConsumption() - feature2.maxConsumption() <<
                        " consumptions over on this feature." << std::endl;
                }
                else
                {
                    //If the user is not in max overages, you can have your normal code.
                    std::cout << "You have " << feature2.maxConsumption() - feature2.totalConsumption() <<
                        " consumptions left on this feature, before you are in the overage territory." << std::endl;
                }
                std::cout << "Starting fibonacci game..." << std::endl;
                fib_game( 20 );
                license->updateFeatureConsumption( "XXXXXX", 1, true );
            }
            catch ( NotEnoughConsumptionException ) 
            {
                std::cout << "You are out of consumptions on this feature. "
                    << "Please reset for more consumptions on this feature." << std::endl;
            }
            catch ( InvalidLicenseFeatureException )
            {
                std::cout << "Could not find feature. Feature does not exist. "
                    << "Please make sure you inputted the correct feature code and that your feature "
                    << "exists on your license." << std::endl;
            }
            catch ( LicenseSpringException ex )
            {
                std::cout << ex.what() << std::endl;
                return 0;
            }
        }
        else if ( sInput.compare( "e" ) != 0 )
        {
            std::cout << "Unrecognized command." << std::endl;
        }
    }
    return 0;
}


//Fibonacci calculator. Returns the n-th term of the fibonacci sequence, where
// n = 0 returns 0, n = 1 returns 1, and any number after n = 1 returns the summation
//of the term n-1 and n-2. Returns -1 for a negative/invalid number.
int fib( int n )
{
    if ( n < 0 )
    {
        std::cout << "No negative fibonacci sequence." << std::endl;
        return -1;
    }
    else if ( n == 0 )
    {
        return 0;
    }
    else if ( n == 1 )
    {
        return 1;
    }
    else
    {
        return fib( n - 1 ) + fib( n - 2 );
    }
}


//Fibonacci game. Given a fibonacci term, the user must figure out what the corresponding fibonacci
//number is. Where term 0 = 0, term 1 = 1, term 2 = 1 and so on... The game ends when the user inputs
//an incorrect answer, other wise it'll continue with different terms. 
//max_terms decides what is the largest fibonacci term that the game may ask you to calculate.
void fib_game( int max_term ) 
{
    std::string sInput = "";
    std::cout << "Type in the correct fibonacci term, given the fibonacci number." << std::endl;
    std::cout << "Note: term 0 = 0, term 1 = 1, term 2 = 1 and so on..." << std::endl;
    time_t Time;
    while( 1 ) 
    {
        srand( (unsigned) time( &Time ) );
        int term =  rand()  % max_term;
        int fib_number = fib( term );

        std::cout << term << std::endl;
        std::cout << ">";

        std::getline( std::cin, sInput );
        try
        {
            if (stoi(sInput) != fib_number)
            {
                std::cout << "Incorrect, the correct number was: " << fib_number << std::endl;
                std::cout << "Exiting game, try to be better next time..." << std::endl;
                return;
            }
            else
            {
                std::cout << "Correct!" << std::endl;
                std::cout << "Next term: " << std::endl;
            }
        }
        catch (std::invalid_argument)
        {
            std::cout << "Invalid argument, please try again with a number next time." << std::endl;
            std::cout << "Exiting game, try to be better next time..." << std::endl;
            return;
        }
        
    }
}
