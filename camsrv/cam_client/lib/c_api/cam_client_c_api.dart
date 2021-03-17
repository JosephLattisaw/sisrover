library c_api;

import 'package:flutter/material.dart';
import 'dart:ffi' as ffi;

//FFI signature for void function
typedef Void_Function_FFI = ffi.Void Function();
//Dart type definition for calling the C foreign function
typedef Void_Function_C = void Function();

////////////////////////////////////////////////////////////////////////
typedef NativeCreateCamClient = ffi.Void Function(ffi.Int32);
typedef NativeCreateCamClientType = ffi.Void Function(
    ffi.Pointer<ffi.NativeFunction<NativeCreateCamClient>>);
typedef CreateCamClientType = void Function(
    ffi.Pointer<ffi.NativeFunction<NativeCreateCamClient>>);
////////////////////////////////////////////////////////////////////////

class CamClientCAPI extends ChangeNotifier {
  ffi.DynamicLibrary _backend;
  static const String _LIBRARY_NAME =
      '/home/efsi/projects/dev/sisrover/repos/sisrover.git/camsrv/cam_client/cam_client_backend/build/libcam_client_backend.so';

  static const String _CREATE_CAM_CLIENT_SYMBOL_NAME = 'create_cam_client';
  static const String _DESTROY_CAM_CLIENT_SYMBOL_NAME = 'destroy_cam_client';
  static const String _RUN_IO_SERVICE_SYMBOL_NAME = 'run_service';

  Void_Function_C _createCamClient;
  Void_Function_C _destroyCamClient;
  Void_Function_C runIOService;

  ffi.Pointer<ffi.NativeFunction<NativeCreateCamClient>> _pointer3;
  CreateCamClientType createc;

  CamClientCAPI() {
    camClientCAPI = this;

    //try to open our dynamic library
    //TODO find out more about this entire try/catch statement for dynamiclibrary
    try {
      //TODO this should always be in the same directory or something similar
      _backend = ffi.DynamicLibrary.open(_LIBRARY_NAME);
    } catch (e) {
      print(e.toString()); //TODO find out if throw prints or this is needed
      throw e;
    }
    assert(_backend != null); //sanity check
    print("flutter: loaded " + _LIBRARY_NAME);

    try {
      ffi.Pointer<ffi.NativeFunction<NativeCreateCamClientType>> pointer =
          _backend.lookup(_CREATE_CAM_CLIENT_SYMBOL_NAME);
      createc = pointer.asFunction();
    } catch (e) {
      throw e;
    }

    print("loaded symbol: " + _CREATE_CAM_CLIENT_SYMBOL_NAME);

    try {
      _destroyCamClient = _backend
          .lookup<ffi.NativeFunction<Void_Function_FFI>>(
              _DESTROY_CAM_CLIENT_SYMBOL_NAME)
          .asFunction();
    } catch (e) {
      throw e;
    }

    print("loaded symbol: " + _DESTROY_CAM_CLIENT_SYMBOL_NAME);

    try {
      runIOService = _backend
          .lookup<ffi.NativeFunction<Void_Function_FFI>>(
              _RUN_IO_SERVICE_SYMBOL_NAME)
          .asFunction();
    } catch (e) {
      throw e;
    }

    print("loaded symbol: " + _RUN_IO_SERVICE_SYMBOL_NAME);

    //_createCamClient(); //starting our backend

    _pointer3 = ffi.Pointer.fromFunction(connection);

    createc(_pointer3);

    runIOService(); //starting the backends asynchronous service
  }

  void dispose() {
    //TODO looks like we don't need to dispose
    //_destroyCamClient();
  }
}

CamClientCAPI camClientCAPI;

void connection(int a) {
  //dart has no true conversion to boolean
  //can't make this a boolean function either because Dart doesn't support
  //boolean callbacks between C
  final bool s = (a == 0 ? false : true);
  print("connection!!! $s");
}
