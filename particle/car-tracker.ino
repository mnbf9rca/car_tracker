// This #include statement was automatically added by the Particle IDE.
#include "AssetTracker.h"

/*
based on the sample libraries for the particle electron asset tracker

simply sends locaiton every 60 seconds to particle
by default, it sends all locaitons
but it can be set to send onyl when the location has changed signficiantly enough


*/


// Set whether you want the device to publish data to the internet by default here.
// 1 will Particle.publish AND Serial.print, 0 will just Serial.print
// Extremely useful for saving data while developing close enough to have a cable plugged in.
// You can also change this remotely using the Particle.function "tmode" defined in setup()
int transmittingData = 1;
double lastLat = 0.0; // holds the last latitude (in degrees)
double lastLon = 0.0; // holds the last longitude degrees measured
int gpsprecision = 3; // how many decimal places for the GPS precision
int transmitAllLocations = 1; // do we transmit all locations, or only when changed

// Used to keep track of the last time we published data
long lastPublish = 0;

// How many minutes between publishes? 10+ recommended for long-time continuous publishing!
int delayMinutes = 1;

// Creating an AssetTracker named 't' for us to reference
AssetTracker t = AssetTracker();

// A FuelGauge named 'fuel' for checking on the battery state
FuelGauge fuel;

// setup() and loop() are both required. setup() runs once when the device starts
// and is used for registering functions and variables and initializing things
void setup() {
    // Sets up all the necessary AssetTracker bits
    t.begin();
    
    // Enable the GPS module. Defaults to off to save power. 
    // Takes 1.5s or so because of delays.
    t.gpsOn();
    
    // Opens up a Serial port so you can listen over USB
    Serial.begin(9600);
    
    // These three functions are useful for remote diagnostics. Read more below.
    Particle.function("tmode", transmitMode);
    Particle.function("batt", batteryStatus);
    Particle.function("gps", gpsPublish);
    Particle.function("tFreq", tFreq);
    Particle.function("gpsPrecision", gpsPrecision);
    Particle.function("tAll", tAll);
    Particle.publish("Startup", "complete", 60, PRIVATE);
}

// loop() runs continuously
void loop() {
    // You'll need to run this every loop to capture the GPS output
    t.updateGPS();

    // if the current time - the last time we published is greater than your set delay...
    if(millis()-lastPublish > delayMinutes*60*1000){
        // Remember when we published
        lastPublish = millis();
        
        // GPS requires a "fix" on the satellites to give good data,
        // so we should only publish data if there's a fix
        if(t.gpsFix()){
            // Only publish if we're in transmittingData mode 1;
            if(transmittingData){
                
                // check lat + lon
                // see if it's moved from the last time
                // if it has, we'll publish the new location
                 int gpsRoundingFactor = 10 * gpsprecision;
                double lat = (double)roundf(t.readLatDeg() * gpsRoundingFactor) / gpsRoundingFactor;
                double lon = (double)roundf(t.readLonDeg() * gpsRoundingFactor) / gpsRoundingFactor;
                
                if ((lat != lastLat) || (lon != lastLon))
                {

                    
                    publishLocation();
                  
                    lastLat = lat;
                    lastLon = lon;
                }
                else
                {
                   
                   if (transmitAllLocations)
                   {
                     publishLocation();
                       
                   }

                }
                // now go to deep sleep for delayMinutes seconds
                // commented out for now because it makes it very hard to deploy new firmware
                // System.sleep(SLEEP_MODE_DEEP,delayMinutes*60);
                
                
            }
            // but always report the data over serial for local development
            Serial.println(t.readLatLon());
                    
            
            
        }
        else
        {
            Particle.publish("E", "no fix", 60, PRIVATE);
        }
        

    }
}

void publishLocation()
{
    Particle.publish("G", t.readLatLon(), 60, PRIVATE); 
}

// change the GPS publishing frequency
int tFreq(String command){
    delayMinutes = atoi(command);
    char updateFreq[5];
    itoa(delayMinutes,updateFreq,10);
    Particle.publish("F", updateFreq, 60, PRIVATE);
    return 1;
}

// do we transmit all data or only when location changes
int tAll(String command){
    transmitAllLocations = atoi(command);
    Particle.publish("A", command, 60, PRIVATE);
    return 1;
}

// change the GPS publishing frequency
int gpsPrecision(String command){
    gpsprecision = atoi(command);
    char newPrecision[5];
    itoa(gpsprecision,newPrecision,10);
    Particle.publish("P", newPrecision, 60, PRIVATE);
    return 1;
}

// Allows you to remotely change whether a device is publishing to the cloud
// or is only reporting data over Serial. Saves data when using only Serial!
// Change the default at the top of the code.
int transmitMode(String command){
    transmittingData = atoi(command);
    Particle.publish("T", command, 60, PRIVATE);
    return 1;
}

// Actively ask for a GPS reading if you're impatient. Only publishes if there's
// a GPS fix, otherwise returns '0'
int gpsPublish(String command){
    if(t.gpsFix()){ 
        publishLocation();
        
        // uncomment next line if you want a manual publish to reset delay counter
        // lastPublish = millis();
        return 1;
    }
    else { return 0; }
}

// Lets you remotely check the battery status by calling the function "batt"
// Triggers a publish with the info (so subscribe or watch the dashboard)
// and also returns a '1' if there's >10% battery left and a '0' if below
int batteryStatus(String command){
    // Publish the battery voltage and percentage of battery remaining
    // if you want to be really efficient, just report one of these
    // the String::format("%f.2") part gives us a string to publish,
    // but with only 2 decimal points to save space
    Particle.publish("B", 
          "v:" + String::format("%f.2",fuel.getVCell()) + 
          ",c:" + String::format("%f.2",fuel.getSoC()),
          60, PRIVATE
    );
    // if there's more than 10% of the battery left, then return 1
    if(fuel.getSoC()>10){ return 1;} 
    // if you're running out of battery, return 0
    else { return 0;}
}