import 'package:flutter/material.dart';
import 'package:health_app/screens/chatbot_screen.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Health App',
      debugShowCheckedModeBanner: false,
      home: const ChatbotScreen(),
    );
  }
}