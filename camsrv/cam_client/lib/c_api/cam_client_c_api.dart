library c_api;

import 'dart:isolate';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'dart:ffi' as ffi;

typedef StartWorkType = ffi.Void Function(ffi.Int64 port, ffi.Int64 port2);
typedef StartWorkFunc = void Function(int port, int port2);

//FFI signature for void function
typedef Void_Function_FFI = ffi.Void Function();
//Dart type definition for calling the C foreign function
typedef Void_Function_C = void Function();

class CamClientCAPI {
  static const String _LIBRARY_NAME =
      '/home/efsi/projects/dev/sisrover/repos/sisrover.git/camsrv/cam_client/cam_client_backend/build/libcam_client_backend.so';

  BuildContext context;

  ValueNotifier<bool> connected;
  ValueNotifier<MemoryImage?> image;

  Future<void> sendImage(Uint8List data) async {
    final imageMemory = MemoryImage(data);

    try {
      await precacheImage(imageMemory, context, onError: (err, trace) {
        print(err);
      });
      image.value = imageMemory;
    } catch (ex) {}
  }

  void setConnection(bool status) {
    print("CamClientCAPI: setConnection($status)");
  }

  CamClientCAPI(this.context, this.image, this.connected) {
    camClientCAPI = this;

    final lib = ffi.DynamicLibrary.open(_LIBRARY_NAME);

    final initializeApi = lib.lookupFunction<
        ffi.IntPtr Function(ffi.Pointer<ffi.Void>),
        int Function(ffi.Pointer<ffi.Void>)>("InitializeDartApi");

    if (initializeApi(ffi.NativeApi.initializeApiDLData) != 0) {
      throw "Failed to initialize Dart API";
    }

    ReceivePort connectionPort = ReceivePort()
      ..listen((status) {
        print('connection: status changed to $status');
        connected.value = status;
      });
    int connectionNativePort = connectionPort.sendPort.nativePort;

    ReceivePort imagePort = ReceivePort()
      ..listen((data) async {
        await sendImage(data);
      });
    int imageNativePort = imagePort.sendPort.nativePort;

    StartWorkFunc cam = lib
        .lookup<ffi.NativeFunction<StartWorkType>>("create_cam_client")
        .asFunction();

    Void_Function_C runIOService = lib
        .lookup<ffi.NativeFunction<Void_Function_FFI>>("run_service")
        .asFunction();

    //TODO see if we need this
    Void_Function_C cleanCam = lib
        .lookup<ffi.NativeFunction<Void_Function_FFI>>("destroy_cam_client")
        .asFunction();

    cam(connectionNativePort, imageNativePort);
    runIOService();
  }
}

CamClientCAPI? camClientCAPI;
