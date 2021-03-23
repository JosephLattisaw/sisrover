import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/material.dart';
import 'dart:io' show Platform, Directory;
import 'package:path/path.dart' as path;
import 'c_api/cam_client_c_api.dart' as c_api;

import 'dart:ffi' as ffi; //TODO remove
import 'package:ffi/ffi.dart';
import 'dart:isolate';
import 'dart:io';

import 'package:provider/provider.dart';
import 'package:cam_client/counter.dart';
import 'package:http/http.dart';

import 'package:flutter_hooks/flutter_hooks.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: MyHomePage(),
    );
  }
}

class MyHomePage extends HookWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Flutter Demo Home Page'),
      ),
      body: Column(
        children: [
          Expanded(
            child: Center(
              child: Mjpeg(),
            ),
          ),
          Row(
            children: [
              ElevatedButton(
                onPressed: () {
                  //TODO
                },
                child: Text('Toggle'),
              ),
              ElevatedButton(
                onPressed: () {},
                child: Text('Push new route'),
              ),
            ],
          )
        ],
      ),
    );
  }
}

class Mjpeg extends HookWidget {
  final double? width;
  final double? height;
  final BoxFit? fit;

  const Mjpeg({
    this.width,
    this.height,
    this.fit,
    Key? key,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final image = useState<MemoryImage?>(null);
    final errorState = useState<dynamic>(null);
    final cam_client = useMemoized(() => c_api.CamClientCAPI(context, image));

    if (image.value == null) {
      return SizedBox(
        width: 200.0,
        height: 300.0,
        child: Text('Hello World'),
      );
    }

    return Image(
      image: image.value!,
      width: width,
      height: height,
      fit: fit,
      gaplessPlayback: true,
    );
  }
}
