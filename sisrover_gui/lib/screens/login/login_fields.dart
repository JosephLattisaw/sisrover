import 'package:flutter/material.dart';

class LoginFields extends StatelessWidget {
  final Color enabledBorderColor = Colors.black;
  final Color focusedBorderColor = Colors.blue;
  final Color fillColor = Colors.white;
  final double borderWidth = 2.0;

  final String labelText;
  final IconData icon;
  final bool obscureText;

  LoginFields(
      {@required this.labelText,
      @required this.icon,
      this.obscureText = false});

  @override
  Widget build(BuildContext context) {
    return TextField(
      obscureText: obscureText,
      decoration: InputDecoration(
        enabledBorder: OutlineInputBorder(
            borderSide: BorderSide(
          color: enabledBorderColor,
          width: borderWidth,
        )),
        focusedBorder: OutlineInputBorder(
            borderSide: BorderSide(
          color: focusedBorderColor,
          width: borderWidth,
        )),
        fillColor: fillColor,
        labelText: labelText,
        prefixIcon: Icon(
          icon,
          color: focusedBorderColor,
        ),
      ),
    );
  }
}
