import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

class ChatbotPage extends StatefulWidget {
  const ChatbotPage({super.key});

  @override
  State<ChatbotPage> createState() => _ChatbotPageState();
}

class _ChatbotPageState extends State<ChatbotPage> {
  final List<Map<String, String>> messages = [];
  final TextEditingController _controller = TextEditingController();

  final String cozeUrl = 'https://api.coze.com/open_api/v2/chat';
  final String token = 'pat_KyZngnD9AeJ3UOcWI3cZ8ZtAWYNckOAsr9QeU1tadp8N3ivY6pEwW2pocuQ2UlAT';

  Future<void> _sendMessage() async {
    if (_controller.text.trim().isNotEmpty) {
      final userMessage = _controller.text.trim();

      setState(() {
        messages.add({'sender': 'user', 'text': userMessage});
      });

      _controller.clear();

      try {
        final response = await http.post(
          Uri.parse(cozeUrl),
          headers: {
            'Authorization': 'Bearer $token',
            'Content-Type': 'application/json',
            'Connection': 'keep-alive',
            'Accept': '*/*',
          },
          body: json.encode({
            "conversation_id": "demo-0",
            "bot_id": "7490748196700995592",
            "user": "demo-user",
            "query": userMessage,
            "stream": false,
          }),
        );

        if (response.statusCode == 200) {
          final responseData = json.decode(response.body);
          final botReply = responseData['response'] ?? 'Bot không trả lời được.';

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
      body: Column(
        children: [
          Expanded(
            child: ListView.builder(
              itemCount: messages.length,
              reverse: true,
              itemBuilder: (context, index) {
                final message = messages[messages.length - 1 - index];
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
                      hintText: 'Type your message...',
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