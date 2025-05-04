// Place this file at: Bangalore_v2v/src/bangalore_v2v/MyBangaloreApp.ned

package bangalore_v2v.MyBangaloreApp;  // Package should match directory structure

// Import the base class
import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;

simple MyBangaloreApp extends DemoBaseApplLayer
{
    parameters:
        @class(bangalore_v2v::MyBangaloreApp);  // Namespace should match C++ file

        double broadcastInterval @unit(s) = default(1s);

        // headerLength is inherited from DemoBaseApplLayer
        // int headerLength @unit(bit) = default(88bit);

        bool logVehicleData = default(true);
        string logFileName = default("v2v_log.txt");

        @display("i=block/app2");

    // Gates are inherited from DemoBaseApplLayer
}