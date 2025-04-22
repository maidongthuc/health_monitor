// import 'package:flutter/material.dart';
// import 'package:health_app/screens/welcom_page.dart';

// void main() {
//   runApp(const MyApp());
// }

// class MyApp extends StatelessWidget {
//   const MyApp({super.key});
//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       title: 'Health App',
//       debugShowCheckedModeBanner: false,
//       home: const WelcomeScreen(),
//     );
//   }
// }

import 'package:flutter/material.dart';
import 'screens/chatbot_page.dart';

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
      home: const ChatbotPage(), // Đặt ChatbotPage làm màn hình chính
    );
  }
}
