// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_MESSAGE_START_H_
#define IPC_IPC_MESSAGE_START_H_

#include "pal/ipc/pal_macros.h"

// Used by IPC_BEGIN_MESSAGES so that each message class starts from a unique
// base.  Messages have unique IDs across channels in order for the IPC logging
// code to figure out the message class from its ID.
//
// You should no longer be adding any new message classes. Instead, use mojo
// for all new work.
enum IPCMessageStart {
  AutomationMsgStart = 0,
  FrameMsgStart,
  PageMsgStart,
  ViewMsgStart,
  InputMsgStart,
  TestMsgStart,
  WorkerMsgStart,
  NaClMsgStart,
  GpuChannelMsgStart,
  MediaMsgStart,
  PpapiMsgStart,
  DOMStorageMsgStart,
  SpeechRecognitionMsgStart,
  P2PMsgStart,
  ResourceMsgStart,
  FileSystemMsgStart,
  BlobMsgStart,
  AudioMsgStart,
  MidiMsgStart,
  ChromeMsgStart,
  DragMsgStart,
  PrintMsgStart,
  ExtensionMsgStart,
  TextInputClientMsgStart,
  JavaBridgeMsgStart,
  ShellMsgStart,
  AccessibilityMsgStart,
  PrerenderMsgStart,
  ChromotingMsgStart,
  BrowserPluginMsgStart,
  AndroidWebViewMsgStart,
  MediaPlayerMsgStart,
  TracingMsgStart,
  PeerConnectionTrackerMsgStart,
  AppShimMsgStart,
  WebRtcLoggingMsgStart,
  TtsMsgStart,
  NaClHostMsgStart,
  EncryptedMediaMsgStart,
  ServiceWorkerMsgStart,
  CastMsgStart,
  ChromeExtensionMsgStart,
  GinJavaBridgeMsgStart,
  ChromeUtilityPrintingMsgStart,
  AecDumpMsgStart,
  OzoneGpuMsgStart,
  PlatformNotificationMsgStart,
  LayoutTestMsgStart,
  NetworkHintsMsgStart,
  CastMediaMsgStart,
  SyncCompositorMsgStart,
  ExtensionsGuestViewMsgStart,
  GuestViewMsgStart,
  // Note: CastCryptoMsgStart and CastChannelMsgStart reserved for Chromecast
  // internal code. Contact gunsch@ before changing/removing.
  CastCryptoMsgStart,
  CastChannelMsgStart,
  IPCTestMsgStart,
  MediaPlayerDelegateMsgStart,
  SurfaceViewManagerMsgStart,
  ExtensionWorkerMsgStart,
  SubresourceFilterMsgStart,
  // Added for Pal injection.
  BrowserControlMsgStart,
  // Added for neva injections.
  InjectionMsgStart,
  // Added for neva PAL.
  PalMsgStart,
  LastIPCMsgStart  // Must come last.
};

#endif  // IPC_IPC_MESSAGE_START_H_
