# qmqtt-state-mirror
A simple library for mirroring property state of Qt objects via MQTT and JSON.

# Dependencies
This project uses the QMQTT library. The project can be cloned from https://github.com/interaktionsbyran/qmqtt. Since there's no stable release of either of these projects yet, expect some flux in API and compatability for now.

# Building
Clone the QMQTT project mentioned above and build it as a library of your choosing. Create a local_settings.pri file in the root of the repository (don't worry, it's in the .gitignore) and add the paths for the library and the headers:

```qmake
LIBS += -Lpath/to/<.a/.so/.dylib/.dll file> 
INCLUDEPATH += path/to/folder/containing/
```

# Usage
As mentioned the API is by no means locked down but the general idea is as follows:

1. Define QObjects with properties you wish to mirror on the network, either in QML or C++.
2. Create an instance of the state manager and pass it a QMQTT::Client instance.
3. Register previously defined objects on the statemanager using registerObject(object, topic).

That's it, you're done! Any change to any of the properties made by your application will now cause the properties to be published on the specified topic serialized as json. Should anyone else publish changes on the same topic that conform to your object, these will be set on the object in your application. Currently the library supports no ownership semantics and uses a simple revision counter to determine if a published update should be respected or not. Thread safety has not yet been considered (but will be soon!).

