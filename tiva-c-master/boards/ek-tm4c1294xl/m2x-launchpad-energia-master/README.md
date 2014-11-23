LaunchPad Energia M2X API Client
=====================

The LaunchPad Energia library is used to send/receive data to/from [AT&amp;T's M2X Data Service](https://m2x.att.com/) from [Tiva C Series LaunchPad](http://www.ti.com/ww/en/launchpad/launchpads-connected.html#tabs) based devices.

**NOTE**: Unless stated otherwise, the following instructions are specific to [Tiva C Series EK-TM4C123GXL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c123gxl.html#tabs) and [Tiva C Series EK-TM4C1294XL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c1294xl.html#tabs) boards. If you are using other boards, the exact steps may vary.


Getting Started
==========================
1. Signup for an [M2X Account](https://m2x.att.com/signup).
2. Obtain your _Master Key_ from the Master Keys tab of your [Account Settings](https://m2x.att.com/account) screen.
3. Create your first [Data Source Blueprint](https://m2x.att.com/blueprints) and copy its _Feed ID_.
4. Review the [M2X API Documentation](https://m2x.att.com/developer/documentation/overview).
5. Obtain a TI Tiva C Series Launchpad, either with built in ethernet or with a wifi BoosterPack, and [set it up](http://www.ti.com/lit/ug/spmu296/spmu296.pdf).  These docs were written for [Tiva C Series EK-TM4C123GXL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c123gxl.html#tabs) with [SimpleLink™ Wi-Fi CC3000 BoosterPack](http://www.ti.com/tool/cc3000boost), and [Tiva C Series EK-TM4C1294XL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c1294xl.html#tabs) boards.

Please consult the [M2X glossary](https://m2x.att.com/developer/documentation/glossary) if you have questions about any M2X specific terms.

How to Install the library
==========================

This library is designed for use with the [Energia IDE](http://energia.nu).  Directions for installing and configuring the IDE can be found [here](http://energia.nu/Guide_index.html).

This library depends on [jsonlite](https://github.com/citrusbyte/jsonlite), the installation steps are as follows:

1. Clone the [jsonlite](https://github.com/citrusbyte/jsonlite) repository.

   **NOTE**: Since we are now using the old v1.1.2 API(we will migrate to the new API soon), please use the fork version of jsonlite listed above instead of the original one.

2. Open the Energia IDE, click `Sketch->Add File...`, then navigate to `amalgamated/jsonlite` folder in the cloned jsonlite repository. The jsonlite library will be imported to Energia this way.

   **NOTE**: If you cloned the jsonlite library, there will be 3 folders named jsonlite:
   * `jsonlite`: the repo folder
   * `jsonlite/jsonlite`: the un-flattened jsonlite source folder
   * `jsonlite/amalgamated/jsonlite`: the flattened jsonlite source

   The last one here should be the one to use, the first 2 won't work!
3. Use the instructions outlined in Step 2 above to import the `M2XStreamClient` library in the current folder.
4. Now you can find M2X examples under `File->Examples->M2XStreamClient`
5. Enjoy coding!

Hardware Setup
==============

Board Setup
-----------

The Energia website has a very good [tutorial](http://energia.nu/Guide_index.html) on setting up the LaunchPad board. It contains detailed instructions on how to install the Energia IDE, and sets up your board for initial testing. Feel free to proceed to the [LaunchPad site](http://www.ti.com/ww/en/launchpad/launchpads-connected.html#tabs) to get a basic idea on LaunchPad.

Wifi/Ethernet Shield Setup
--------------------------

If you are using a [Tiva C Series EK-TM4C1294XL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c1294xl.html#tabs) board instead of a [Tiva C Series EK-TM4C123GXL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c123gxl.html#tabs) board, you can skip this section since the EK-TM4C1294XL board already has Ethernet on board.

To send data to the AT&amp;T M2X Data Service, or receive data from the AT&amp;T M2X Data Service, your LaunchPad board needs a connection to the Internet. Hence a LaunchPad [SimpleLink™ Wi-Fi CC3000 BoosterPack](http://www.ti.com/tool/cc3000boost) is needed to give your board the power to connect to the Internet. To install the shield, hook the shield on your LaunchPad board — you can use the pins on the shield the same way as the real pins on the LaunchPad boards.

Sensor Setup
------------

Different sensors can be hooked up to an LaunchPad board to provide different properties including temperatures, humidity, etc. You can use a breadboard as well as wires to connect different sensors to your LaunchPad. For detailed tutorials on connecting different sensors, please refer to the LaunchPad [Wiki page](http://processors.wiki.ti.com/index.php/Tiva_C_Series_TM4C123G_LaunchPad).


Variables used in Examples
==========================

In order to run the given examples, different variables need to be configured. We will walk through those variables in this section.

Network Configuration
---------------------

If you are using a Wifi Shield, the following variables need configuration:

```
char ssid[] = "<ssid>";
char pass[] = "<WPA password>";
```

Just fill in the SSID and password of the Wifi hotspot, you should be good to go.

For an Ethernet Shield, the following variables are needed:

```
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,17);
```

For a newer Ethernet Shield, the MAC address should be printed on a sticker on the shield. However, some sold Ethernet Shields have no MAC address shown on the board. In this case, you can use any MAC address, as long as it is not conflicted with another network device within the same LAN.

The IP address here is only used when DHCP fails to give a valid IP address. It is recommended, though not required, to provide a unique IP address here.

M2X API Key
-----------

Once you [register](https://m2x.att.com/signup) for an AT&amp;T M2X account, an API key is automatically generated for you. This key is called a _Primary Master Key_ and can be found in the _Master Keys_ tab of your [Account Settings](https://m2x.att.com/account). This key cannot be edited nor deleted, but it can be regenerated. It will give you full access to all APIs.

However, you can also create a _Data Source API Key_ associated with a given Data Source(Feed), you can use the Data Source API key to access the streams belonging to that Data Source.

You can customize this variable in the following line in the examples:

```
char m2xKey[] = "<M2X access key>";
```

Feed ID
-------

A feed is associated with a data source, it is a set of data streams, such as streams of locations, temperatures, etc. The following line is needed to configure the feed used:

```
char feedId[] = "<feed id>";
```

Stream Name
------------

A stream in a feed is a set of timed series data of a specific type(i,e. humidity, temperature), you can use the M2XStreamClient library to send stream values to M2X server, or receive stream values from M2X server. Use the following line to configure the stream if needed:

```
char streamName[] = "<stream name>";
```

Using the M2XStreamClient library
=========================

The M2X LaunchPad library can be used with both Wifi connection and Ethernet connection. For a Wifi connection, use the following code:

```
WiFiClient client;
M2XStreamClient m2xClient(&client, m2xKey);
```

For an Ethernet connection, use the following code:

```
EthernetClient client;
M2XStreamClient m2xClient(&client, m2xKey);
```


In the M2XStreamClient, 4 types of API functions are provided here:

* `send`: Send stream value to M2X server
* `receive`: Receive stream value from M2X server
* `updateLocation`: Send location value of a feed to M2X server
* `readLocation`: Receive location values of a feed from M2X server

Returned values
---------------

For all those functions, the HTTP status code will be returned if we can fulfill a HTTP request. For example, `200` will be returned upon success, `401` will be returned if we didn't provide a valid M2X API Key.

Otherwise, the following error codes will be used:

```
static const int E_NOCONNECTION = -1;
static const int E_DISCONNECTED = -2;
static const int E_NOTREACHABLE = -3;
static const int E_INVALID = -4;
static const int E_JSON_INVALID = -5;
```

Post stream value
-----------------

The following functions can be used to post value to a stream, which belongs to a feed:

```
template <class T>
int post(const char* feedId, const char* streamName, T value);
```

Here we use C++ templates to generate functions for different types of values, feel free to use values of `float`, `int`, `long` or even `const char*` types here.

Post multiple values
--------------------

M2X also supports posting multiple values to multiple streams in one call, use the following function for this:

```
template <class T>
int postMultiple(const char* feedId, int streamNum,
                 const char* names[], const int counts[],
                 const char* ats[], T values[]);
```

Please refer to the comments in the source code on how to use this function, basically, you need to provide the list of streams you want to post to, and values for each stream.

Fetch stream value
------------------

Since your board contains limited memory, we cannot put the whole returned string in memory, parse it into JSON representations and read what we want. Instead, we use a callback-based mechanism here. We parse the returned JSON string piece by piece, whenever we got a new stream value point, we will call the following callback functions:

```
void (*stream_value_read_callback)(const char* at,
                                   const char* value,
                                   int index,
                                   void* context);
```

The implementation of the callback function is left for the user to fill in, you can read the value of the point in the `value` argument, and the timestamp of the point in the `at` argument. We even pass the index of this this data point in the whole stream as well as a user-specified context variable to this function, so as you can perform different tasks on this.

To read the stream values, all you need to do is calling this function:

```
int fetchValues(const char* feedId, const char* streamName,
                stream_value_read_callback callback, void* context,
                const char* startTime = NULL, const char* endTime = NULL,
                const char* limit = NULL);
```

Besides the feed ID and stream name, only the callback function and a user context needs to be specified. Optional filtering parameters such as start time, end time and limits per call can also be used here.

Update Datasource Location
--------------------------

You can use the following function to update the location for a data source(feed):

```
template <class T>
int updateLocation(const char* feedId, const char* name,
                   T latitude, T longitude, T elevation);
```

Different from stream values, locations are attached to feeds rather than streams.

The reasons we are providing templated function is due to floating point value precision: on most boards, `double` is the same as `float`, i.e., 32-bit (4-byte) single precision floating number. That means only 7 digits in the number is reliable. When we are using `double` here to represent latitude/longitude, it means only 5 digits after the floating point is accurate, which means we can represent as accurate to ~1.1132m distance using `double` here. If you want to represent cordinates that are more specific, you need to use strings here.

Read Datasource Location
------------------------

Similar to reading stream values, we also use callback functions here. The only difference is that different parameters are used in the function:

```
void (*location_read_callback)(const char* name,
                               double latitude,
                               double longitude,
                               double elevation,
                               const char* timestamp,
                               int index,
                               void* context);
```

For memory space consideration, now we only provide double-precision when reading locations. An index of the location points is also provided here together with a user-specified context.

The API is also slightly different, in that the stream name is not needed here:

```
int readLocation(const char* feedId, location_read_callback callback,
                 void* context);
```

Examples
========

We provide a series of examples that will help you get an idea of how to use the `M2XStreamClient` library to perform all kinds of tasks.

Note that the examples may apply to certain types of boards. For example, the ones with `Wifi` in the name apply to `Tiva C Series` boards with a `CC3300 BoosterPack`, while the ones with `Ethernet` apply to `Tiva C Series` boards with built-in ethernet.

Note that the examples contain fictionary variables, and that they need to be configured as per the instructions above before running on your LaunchPad board. Each of the examples here also needs either a Wifi Shield or an Ethernet Shield hooked up to your device.

In the `LaunchPadWifiPost` and `LaunchPadEthernetPost`, a temperature sensor, a breadboard and 5 wires are also needed to get temperature data, you need to wire the board like [this](http://cl.ly/image/3M0P3T1A0G0l) before running the code.

After you have configured your variables and the board, plug the LaunchPad board into your computer via a Micro-USB cable, click `Verify` in the Energia IDE, then click `Upload`, and the code should be uploaded to the board. You can check all the outputs in the `Serial Monitor` of the Energia IDE.

LaunchPadWifiPost
-----------------

This example shows how to post temperatures to M2X server. Before running this, you need to have a valid M2X Key, a feed ID and a stream name. The LaunchPad board needs to be configured like [this](http://cl.ly/image/3M0P3T1A0G0l). In this example, we are using a [Tiva C Series EK-TM4C123GXL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c123gxl.html#tabs) with [SimpleLink™ Wi-Fi CC3000 BoosterPack](http://www.ti.com/tool/cc3000boost) board. If you are using other boards, keep in mind that we are reading from `A0` in the code, the wiring should be similar to this one shown in the illustration.

LaunchPadWifiPostMultiple
---------------

This example shows how to post multiple values to multiple streams in one API call.

LaunchPadWifiFetchValues
--------------

This example reads stream values from M2X server. And prints the stream data point got to Serial interface. You can find the actual values in the `Serial Monitor`.

LaunchPadWifiUpdateLocation
-----------------

This one sends location data to M2X server. Idealy a GPS device should be used here to read the cordinates, but for simplicity, we just use pre-set values here to show how to use the API.

LaunchPadWifiReadLocation
---------------

This one reads location data of a feed from M2X server, and prints them to Serial interfact. You can check the output in the `Serial Monitor` in your IDE.

LaunchPadEthernetPost
---------------

This one is similar to the `LaunchPadWifiPost`, except that EthernetClient is used instead of WifiClient. If you are using a [Tiva C Series EK-TM4C1294XL](http://www.ti.com/ww/en/launchpad/launchpads-connected-ek-tm4c1294xl.html#tabs) board, you can use this example.

LaunchPadEthernetReceive
------------------

This one is similar to the `LaunchPadWifiReceive`, except that EthernetClient is used instead of WifiClient.



LICENSE
=======

This library is released under the MIT license. See [`M2XStreamClient/LICENSE`](M2XStreamClient/LICENSE) for the terms.
