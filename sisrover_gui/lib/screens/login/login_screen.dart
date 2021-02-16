import 'package:flutter/material.dart';
import 'package:window_size/window_size.dart';
import 'package:sisrover_gui/screens/login/login_fields.dart';

class MyApp extends StatelessWidget {
  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    setWindowTitle("Smart Imaging Systems, Inc.");
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        primarySwatch: Colors.blue,
      ),
      home: MyHomePage(title: 'XScan CXR'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  MyHomePage({Key key, this.title}) : super(key: key);

  final String title;

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              height: 200,
              decoration: BoxDecoration(
                border: Border.all(
                  color: Colors.black,
                  width: 2.5,
                ),
              ),
              child: Column(
                children: [
                  Container(
                    color: Colors.blue[900],
                    child: Text(
                      "SIS Scan Login",
                      style: TextStyle(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                        fontSize: 25.0,
                      ),
                    ),
                  ),
                  Container(
                    child: Column(
                      children: [
                        LoginFields(labelText: "Username", icon: Icons.person),
                        SizedBox(
                          height: 7,
                        ),
                        LoginFields(
                          labelText: "Password",
                          icon: Icons.lock,
                          obscureText: true,
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            )
          ],
        ),
      ),
    );
  }
}
