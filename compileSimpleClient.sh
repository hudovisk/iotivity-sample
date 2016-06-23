g++ simple_client.cpp \
-I $IOTIVITY_HOME/resource/csdk/stack/include/ \
-I $IOTIVITY_HOME/resource/c_common/ \
-I $IOTIVITY_HOME/resource/include/ \
-I $IOTIVITY_HOME/resource/oc_logger/include/ \
-L $IOTIVITY_HOME/out/linux/x86_64/release \
-I $SOCKETIO_HOME/src -L $SOCKETIO_HOME/ \
-std=c++0x -loctbstack -loc_logger -loc -lsioclient -lpthread -lboost_system 
