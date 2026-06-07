import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/foundation.dart';

class DefaultFirebaseOptions {
  static FirebaseOptions get currentPlatform {
    if (kIsWeb) {
      return web;
    }

    return android;
  }

  static const FirebaseOptions web = FirebaseOptions(
    apiKey: 'AIzaSyCv8KHPU8kA7CtMgQdP3pnuGKqz4ZXhCYc',
    appId: '1:625804821731:web:8b29c7a883512ef1c131d6',
    messagingSenderId: '625804821731',
    projectId: 'reefcontrol-platform',
    storageBucket: 'reefcontrol-platform.firebasestorage.app',
  );

  static const FirebaseOptions android = FirebaseOptions(
    apiKey: 'AIzaSyCv8KHPU8kA7CtMgQdP3pnuGKqz4ZXhCYc',
    appId: '1:625804821731:android:abbf45978d2d6680c131d6',
    messagingSenderId: '625804821731',
    projectId: 'reefcontrol-platform',
    storageBucket: 'reefcontrol-platform.firebasestorage.app',
  );
}