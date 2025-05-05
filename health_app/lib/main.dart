import 'package:flutter/material.dart';
import 'screens/chatbot_page.dart'; // Import ChatbotScreen

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Health Monitor',
      theme: ThemeData(primarySwatch: Colors.blue),
      home: const ChatbotScreen(), // Đặt ChatbotScreen làm màn hình chính
    );
  }
}