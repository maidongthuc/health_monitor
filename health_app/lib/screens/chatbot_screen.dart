import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

class ChatbotScreen extends StatefulWidget {
  const ChatbotScreen({super.key});

  @override
  State<ChatbotScreen> createState() => _ChatbotScreenState();
}

class _ChatbotScreenState extends State<ChatbotScreen> {
  final List<Map<String, String>> messages = [];
  final TextEditingController _controller = TextEditingController();

  Future<void> _sendMessage() async {
    if (_controller.text.trim().isNotEmpty) {
      final userMessage = _controller.text.trim();

      setState(() {
        messages.add({'sender': 'user', 'text': userMessage});
      });

      _controller.clear();

      try {
        // Gửi yêu cầu POST đến API
        final response = await http.post(
          Uri.parse('http://127.0.0.1:3000/api/chat/completions'),
          headers: {
            'Authorization':
                'Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6ImFlZTI3YTRhLWJlNWQtNDM5NC1hMGUzLTQ3MTZkM2NlNjY1MyJ9.9NKkhtYbM5OXbJx42S8dQMssmXVz8u6hVSgVMAssvLg',
            'Content-Type': 'application/json',
          },
          body: json.encode({
            'model': 'sthealthy',
            'messages': [
              {'role': 'user', 'content': userMessage}
            ],
          }),
        );

        if (response.statusCode == 200) {
          final responseData = json.decode(utf8.decode(response.bodyBytes)); // Đảm bảo không lỗi font
          final botReply = responseData['choices']?[0]?['message']?['content'] ??
              'Bot không trả lời được.';

          setState(() {
            messages.add({'sender': 'bot', 'text': botReply});
          });
        } else {
          setState(() {
            messages.add({'sender': 'bot', 'text': 'Lỗi: Không thể kết nối đến API.'});
          });
        }
      } catch (e) {
        setState(() {
          messages.add({'sender': 'bot', 'text': 'Lỗi: Đã xảy ra sự cố.'});
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Chatbot'),
      ),
      body: Column(
        children: [
          Expanded(
            child: ListView.builder(
              itemCount: messages.length,
              itemBuilder: (context, index) {
                final message = messages[index];
                final isUser = message['sender'] == 'user';
                return Align(
                  alignment: isUser ? Alignment.centerRight : Alignment.centerLeft,
                  child: Container(
                    margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 10),
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: isUser ? Colors.blue[100] : Colors.grey[300],
                      borderRadius: BorderRadius.circular(10),
                    ),
                    child: Text(
                      message['text']!,
                      style: const TextStyle(fontSize: 16),
                    ),
                  ),
                );
              },
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(8.0),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _controller,
                    decoration: const InputDecoration(
                      hintText: 'Nhập tin nhắn...',
                      border: OutlineInputBorder(),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                IconButton(
                  icon: const Icon(Icons.send, color: Colors.blue),
                  onPressed: _sendMessage,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}