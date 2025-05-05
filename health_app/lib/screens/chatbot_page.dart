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
  final List<String> chatHistory = []; // Lưu lịch sử các cuộc trò chuyện
  final TextEditingController _controller = TextEditingController();
  bool isSidebarVisible = false; // Trạng thái hiển thị sidebar

  // Hàm lưu lịch sử cuộc trò chuyện
  Future<void> _saveChatHistory(String userMessage, String botReply) async {
    try {
      final response = await http.post(
        Uri.parse('http://127.0.0.1:5000/history_chat'),
        headers: {
          'Content-Type': 'application/json',
        },
        body: json.encode({
          'user': userMessage,
          'assistant': botReply,
        }),
      );

      if (response.statusCode == 200) {
        print('Lịch sử cuộc trò chuyện đã được lưu.');
      } else {
        print('Lỗi khi lưu lịch sử: ${response.body}');
      }
    } catch (e) {
      print('Lỗi khi gửi yêu cầu lưu lịch sử: $e');
    }
  }

  // Hàm gửi tin nhắn đến API chatbot
  Future<void> _sendMessage() async {
    if (_controller.text.trim().isNotEmpty) {
      final userMessage = _controller.text.trim();

      setState(() {
        messages.add({'role': 'user', 'content': userMessage});
      });

      _controller.clear();

      try {
        final response = await http.post(
          Uri.parse('http://localhost:3000/api/chat/completions'),
          headers: {
            'Authorization': 'Bearer sk-7462a3aa19e941d7ae7881b923542ff7',
            'Content-Type': 'application/json',
          },
          body: json.encode({
            'stream': false,
            'model': 'veronai',
            'messages': messages,
          }),
        );

        if (response.statusCode == 200) {
          final responseData = json.decode(utf8.decode(response.bodyBytes));
          final botReply = responseData['choices']?[0]?['message']?['content'] ??
              'Bot không trả lời được.';

          setState(() {
            messages.add({'role': 'assistant', 'content': botReply});
          });

          // Lưu lịch sử cuộc trò chuyện
          await _saveChatHistory(userMessage, botReply);

          // Thêm vào danh sách lịch sử
          chatHistory.add(userMessage);
        } else {
          setState(() {
            messages.add({'role': 'assistant', 'content': 'Lỗi: Không thể kết nối đến API.'});
          });
        }
      } catch (e) {
        setState(() {
          messages.add({'role': 'assistant', 'content': 'Lỗi: Đã xảy ra sự cố.'});
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.blueAccent,
        title: const Text(
          'Chatbot',
          style: TextStyle(fontWeight: FontWeight.bold),
        ),
        centerTitle: true,
        leading: IconButton(
          icon: const Icon(Icons.menu),
          onPressed: () {
            setState(() {
              isSidebarVisible = !isSidebarVisible; // Toggle trạng thái sidebar
            });
          },
        ),
      ),
      body: Stack(
        children: [
          // Khu vực chính hiển thị tin nhắn
          Row(
            children: [
              if (isSidebarVisible)
                Container(
                  width: 250,
                  color: Colors.grey[200],
                  child: Column(
                    children: [
                      const Padding(
                        padding: EdgeInsets.all(16.0),
                        child: Text(
                          'Lịch sử trò chuyện',
                          style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
                        ),
                      ),
                      Expanded(
                        child: ListView.builder(
                          itemCount: chatHistory.length,
                          itemBuilder: (context, index) {
                            return ListTile(
                              leading: const Icon(Icons.chat, color: Colors.blueAccent),
                              title: Text(
                                chatHistory[index],
                                maxLines: 1,
                                overflow: TextOverflow.ellipsis,
                              ),
                              onTap: () {
                                // Xử lý khi nhấn vào lịch sử
                                print('Nhấn vào: ${chatHistory[index]}');
                              },
                            );
                          },
                        ),
                      ),
                    ],
                  ),
                ),
              Expanded(
                child: Column(
                  children: [
                    Expanded(
                      child: Container(
                        decoration: const BoxDecoration(
                          color: Color(0xFFF5F5F5),
                        ),
                        child: ListView.builder(
                          itemCount: messages.length,
                          itemBuilder: (context, index) {
                            final message = messages[index];
                            final isUser = message['role'] == 'user';
                            return Row(
                              mainAxisAlignment:
                                  isUser ? MainAxisAlignment.end : MainAxisAlignment.start,
                              children: [
                                Container(
                                  margin: const EdgeInsets.symmetric(vertical: 5, horizontal: 10),
                                  padding: const EdgeInsets.all(10),
                                  decoration: BoxDecoration(
                                    color: isUser ? Colors.blueAccent : Colors.grey[300],
                                    borderRadius: BorderRadius.circular(20),
                                  ),
                                  child: Text(
                                    message['content'] ?? '',
                                    style: TextStyle(
                                      color: isUser ? Colors.white : Colors.black,
                                    ),
                                  ),
                                ),
                              ],
                            );
                          },
                        ),
                      ),
                    ),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 10),
                      decoration: const BoxDecoration(
                        color: Colors.white,
                        boxShadow: [
                          BoxShadow(
                            color: Colors.black12,
                            blurRadius: 4,
                            offset: Offset(0, -2),
                          ),
                        ],
                      ),
                      child: Row(
                        children: [
                          Expanded(
                            child: TextField(
                              controller: _controller,
                              decoration: InputDecoration(
                                hintText: 'Nhập tin nhắn...',
                                filled: true,
                                fillColor: Colors.grey[200],
                                border: OutlineInputBorder(
                                  borderRadius: BorderRadius.circular(20),
                                  borderSide: BorderSide.none,
                                ),
                                contentPadding:
                                    const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                              ),
                            ),
                          ),
                          const SizedBox(width: 8),
                          GestureDetector(
                            onTap: _sendMessage,
                            child: Container(
                              padding: const EdgeInsets.all(12),
                              decoration: BoxDecoration(
                                color: Colors.blueAccent,
                                shape: BoxShape.circle,
                              ),
                              child: const Icon(Icons.send, color: Colors.white),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}