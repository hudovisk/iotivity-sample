g++ simple_client.cpp -I /home/gabriel/iotivity/resource/csdk/stack/include/ -I /home/gabriel/iotivity/resource/c_common/ -I /home/gabriel/iotivity/resource/include/ -I /home/gabriel/iotivity/resource/oc_logger/include/ -I $SOCKETIO_HOME/src -L $SOCKETIO_HOME/ -std=c++0x -loctbstack -loc_logger -loc -lsioclient -lpthread -lboost_system 

