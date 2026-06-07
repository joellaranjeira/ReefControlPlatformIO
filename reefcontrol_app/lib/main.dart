import 'firebase_options.dart';
import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  await Firebase.initializeApp(
  options: DefaultFirebaseOptions.currentPlatform,
);

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  Future<void> criarTeste() async {
   try {
    await FirebaseFirestore.instance.collection('teste').add({
      'mensagem': 'ReefControl funcionando',
      'data': DateTime.now().toString(),
    });

    print('GRAVADO COM SUCESSO');
  } catch (e) {
    print('ERRO FIRESTORE: $e');
  }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'ReefControl',
      home: Scaffold(
        appBar: AppBar(
          title: const Text('ReefControl'),
        ),
        body: Center(
          child: ElevatedButton(
            onPressed: criarTeste,
            child: const Text('Gravar no Firestore'),
          ),
        ),
      ),
    );
  }
}