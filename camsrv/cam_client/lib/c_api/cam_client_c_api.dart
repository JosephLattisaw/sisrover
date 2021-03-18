library c_api;

import 'dart:isolate';

import 'package:flutter/material.dart';
import 'dart:ffi' as ffi;

typedef StartWorkType = ffi.Void Function(ffi.Int64 port, ffi.Int64 port2);
typedef StartWorkFunc = void Function(int port, int port2);

//FFI signature for void function
typedef Void_Function_FFI = ffi.Void Function();
//Dart type definition for calling the C foreign function
typedef Void_Function_C = void Function();

class CamClientCAPI extends ChangeNotifier {
  ffi.DynamicLibrary lib;
  final String _LIBRARY_NAME =
      '/home/efsi/projects/dev/sisrover/repos/sisrover.git/camsrv/cam_client/cam_client_backend/build/libcam_client_backend.so';

  dynamic initializeApi;

  //connection port
  ReceivePort connectionPort;
  int connectionNativePort;

  //image frames port
  ReceivePort imagePort;
  int imageNativePort;

  StartWorkFunc cam;
  Void_Function_C runIOService;
  Void_Function_C cleanCam;

  CamClientCAPI() {
    camClientCAPI = this;

    lib = ffi.DynamicLibrary.open(_LIBRARY_NAME);

    initializeApi = lib.lookupFunction<
        ffi.IntPtr Function(ffi.Pointer<ffi.Void>),
        int Function(ffi.Pointer<ffi.Void>)>("InitializeDartApi");

    if (initializeApi(ffi.NativeApi.initializeApiDLData) != 0) {
      throw "Failed to initialize Dart API";
    }

    connectionPort = ReceivePort()
      ..listen((status) {
        print('connection: status changed to $status');
      });
    connectionNativePort = connectionPort.sendPort.nativePort;

    imagePort = ReceivePort()
      ..listen((size) {
        print('image: received frame size of ${size.length}');
      });
    imageNativePort = imagePort.sendPort.nativePort;

    cam = lib
        .lookup<ffi.NativeFunction<StartWorkType>>("create_cam_client")
        .asFunction();

    runIOService = lib
        .lookup<ffi.NativeFunction<Void_Function_FFI>>("run_service")
        .asFunction();

    cleanCam = lib
        .lookup<ffi.NativeFunction<Void_Function_FFI>>("destroy_cam_client")
        .asFunction();

    cam(connectionNativePort, imageNativePort);
    runIOService();
  }

  @override
  void dispose() {
    print("attempting to dispose");
    //cleanCam();
    //print("cleaning cam");
    super.dispose();
  }
}

CamClientCAPI camClientCAPI;
