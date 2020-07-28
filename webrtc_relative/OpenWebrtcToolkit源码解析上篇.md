
# Open webrtc toolkit源码解析(上篇)

### 珍惜他人劳动成果，转载请注明[原文地址](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/OpenWebrtcToolkit%E6%BA%90%E7%A0%81%E8%A7%A3%E6%9E%90%E4%B8%8A%E7%AF%87.md)

#### [Owt](https://github.com/open-webrtc-toolkit)(Open webrtc toolkit) 是intel开源基于webrtc的旨在提供简易的音视频通话库，intel对其进行优化并增强了对处理器的优化意在展示强大的芯片处理能力，包含了跨端跨平台从终端到服务器的实现。本篇文章旨在通过分析owt源码流程来梳理webrtc在多人音视频通话中的作用。

### OWTConferenceClient

client端主要api调用在OWTConferenceClient中

1.使用配置创建client实例

```Objc
- (instancetype)initWithConfiguration:(OWTConferenceClientConfiguration*)config;
```

实现文件OWTConferenceClient中

```Objc
- (instancetype)initWithConfiguration:
    (OWTConferenceClientConfiguration*)config {
  self = [super init];
  owt::conference::ConferenceClientConfiguration* nativeConfig =
      new owt::conference::ConferenceClientConfiguration();
  std::vector<owt::base::IceServer> iceServers;
  for (RTCIceServer* server in config.rtcConfiguration.iceServers) {
    owt::base::IceServer iceServer;
    iceServer.urls = server.nativeServer.urls;
    iceServer.username = server.nativeServer.username;
    iceServer.password = server.nativeServer.password;
    iceServers.push_back(iceServer);
  }
  nativeConfig->ice_servers = iceServers; // 1.保存iceservers
  nativeConfig->candidate_network_policy =  // 2.保存candidate策略
      config.rtcConfiguration.candidateNetworkPolicy ==
              RTCCandidateNetworkPolicyLowCost
          ? owt::base::ClientConfiguration::CandidateNetworkPolicy::kLowCost
          : owt::base::ClientConfiguration::CandidateNetworkPolicy::kAll;
  _nativeConferenceClient =
      owt::conference::ConferenceClient::Create(*nativeConfig); //3.创建client实例
  _publishedStreams = [[NSMutableDictionary alloc] init];
  return self;
}
```


其中第三部分到对应的实现文件conferenceClient中

```cpp
std::shared_ptr<ConferenceClient> ConferenceClient::Create(
    const ConferenceClientConfiguration& configuration) {
  return std::shared_ptr<ConferenceClient>(new ConferenceClient (configuration)); // 使用构造函数构造conferenceClient实例
}

// conferenceClient构造函数
ConferenceClient::ConferenceClient(
    const ConferenceClientConfiguration& configuration)
    : configuration_(configuration),
      signaling_channel_(new ConferenceSocketSignalingChannel()), //初始化信令通道
      signaling_channel_connected_(false) { 
  auto task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
  event_queue_ =
      std::make_unique<rtc::TaskQueue>(task_queue_factory_->CreateTaskQueue(
          "ConferenceClientEventQueue",
          webrtc::TaskQueueFactory::Priority::NORMAL)); //使用webrtc内部提供的方式生成事件队列
}
```

可以看到主要做的事件都在片段中标注出


2.加入媒体房间
```Objc
- (void)joinWithToken:(NSString*)token
            onSuccess:(nullable void (^)(OWTConferenceInfo*))onSuccess
            onFailure:(nullable void (^)(NSError*))onFailure;
```

实现部分

```cpp
- (void)joinWithToken:(NSString*)token
            onSuccess:(void (^)(OWTConferenceInfo*))onSuccess
            onFailure:(void (^)(NSError*))onFailure {
  if (token == nil) {
    if (onFailure != nil) {
      NSError* err = [[NSError alloc]
          initWithDomain:OWTErrorDomain
                    code:OWTConferenceErrorUnknown
                userInfo:[[NSDictionary alloc]
                             initWithObjectsAndKeys:@"Token cannot be nil.",
                                                    NSLocalizedDescriptionKey,
                                                    nil]];
      onFailure(err);
    }
    return;
  }
 
  const std::string nativeToken = [token UTF8String]; // token 进行utf8解码
  __weak OWTConferenceClient *weakSelf = self;
  _nativeConferenceClient->Join( // 调用conferenceClient join 传入为token 回调为onSuccess，onFailure
      nativeToken,
      [=](std::shared_ptr<owt::conference::ConferenceInfo> info) {
        if (onSuccess != nil)
          onSuccess([[OWTConferenceInfo alloc]
              initWithNativeInfo:info]);
      },
      [=](std::unique_ptr<owt::base::Exception> e) {
        [weakSelf triggerOnFailure:onFailure withException:(std::move(e))];
      });
}
```

解释下来就是转到内部conferenceClient调用了join方法参数是token回调是onSuccess，onFailure


然后进入到conferenceClient join部分

```cpp
void ConferenceClient::Join(
    const std::string& token,
    std::function<void(std::shared_ptr<ConferenceInfo>)> on_success,
    std::function<void(std::unique_ptr<Exception>)> on_failure) {
  if (signaling_channel_connected_) {
    if (on_failure != nullptr) {
      event_queue_->PostTask([on_failure]() {
        std::unique_ptr<Exception> e(
            new Exception(ExceptionType::kConferenceUnknown,
                          "Already connected to conference server."));
        on_failure(std::move(e));
      });
    }
    return;
  }
  std::string token_base64(token);
  if (!StringUtils::IsBase64EncodedString(token)) {
    RTC_LOG(LS_WARNING) << "Passing token with Base64 decoded is deprecated, "
                           "please pass it without modification.";
    token_base64 = rtc::Base64::Encode(token);
  }
  signaling_channel_->AddObserver(*this); //信令通道添加观察者
  signaling_channel_->Connect( // 信令通道调用connect 输入参数为token_base64
      token_base64,
      [=](sio::message::ptr info) {
        signaling_channel_connected_ = true;
        // Get current user's participantId, user ID and role and fill in the
        // ConferenceInfo.
        std::string participant_id, user_id, role;
        if (info->get_map()["id"]->get_flag() != sio::message::flag_string ||
            info->get_map()["user"]->get_flag() != sio::message::flag_string ||
            info->get_map()["role"]->get_flag() != sio::message::flag_string) {
          RTC_LOG(LS_ERROR)
              << "Room info doesn't contain participant's ID/uerID/role.";
          if (on_failure) {
            event_queue_->PostTask([on_failure]() {
              std::unique_ptr<Exception> e(
                  new Exception(ExceptionType::kConferenceUnknown,
                                "Received invalid user info from MCU."));
              on_failure(std::move(e));
            });
          }
          return;
        } else {
          participant_id = info->get_map()["id"]->get_string();
          user_id = info->get_map()["user"]->get_string();
          role = info->get_map()["role"]->get_string();
          const std::lock_guard<std::mutex> lock(conference_info_mutex_);
          if (!current_conference_info_.get()) {
            current_conference_info_.reset(new ConferenceInfo);
            current_conference_info_->self_.reset(
                new Participant(participant_id, role, user_id));
          }
        }
        auto room_info = info->get_map()["room"];
        if (room_info == nullptr ||
            room_info->get_flag() != sio::message::flag_object) {
          RTC_DCHECK(false);
          return;
        }
        if (room_info->get_map()["id"]->get_flag() !=
            sio::message::flag_string) {
          RTC_DCHECK(false);
          return;
        } else {
          current_conference_info_->id_ =
              room_info->get_map()["id"]->get_string();
        }
        // Trigger OnUserJoin for existed users, and also fill in the
        // ConferenceInfo.
        if (room_info->get_map()["participants"]->get_flag() !=
            sio::message::flag_array) {
          RTC_LOG(LS_WARNING) << "Room info doesn't contain valid users.";
        } else {
          auto users = room_info->get_map()["participants"]->get_vector();
          // Make sure |on_success| is triggered before any other events because
          // OnUserJoined and OnStreamAdded should be triggered after join a
          // conference.
          for (auto it = users.begin(); it != users.end(); ++it) {
            TriggerOnUserJoined(*it, true);
          }
        }
        // Trigger OnStreamAdded for existed remote streams, and also fill in
        // the ConferenceInfo.
        if (room_info->get_map()["streams"]->get_flag() !=
            sio::message::flag_array) {
          RTC_LOG(LS_WARNING) << "Room info doesn't contain valid streams.";
        } else {
          auto streams = room_info->get_map()["streams"]->get_vector();
          for (auto it = streams.begin(); it != streams.end(); ++it) {
            RTC_LOG(LS_INFO) << "Find streams in the conference.";
            TriggerOnStreamAdded(*it, true); //这里的处理在current_conference_info_中添加了streams
          }
        }
        // Invoke the success callback before trigger any participant join or
        // stream added message.
        if (on_success) {
          event_queue_->PostTask(
              [on_success, this]() { on_success(current_conference_info_); });
        }
      },
      on_failure);
}
```


该方法总结如下：
1. 验证信令的连接状态对于已连接的情况直接回调错误否则通过验证继续往下执行（signaling_channel_connected_）
2. 验证token格式要能进行base64解码
3. 信令实例signaling_channel_添加观察者为conferenceClient
4. signaling_channel_调用connect方法传入参数为token，回调参数为(sio::message::ptr info)
5. signaling_channel_调用connect方法及获取回调info的流程先按住不表，这里先主要分析join的加入房间流程
6. connect回调中处理解析participant_id、user_id、role放入current_conference_info_中
7. 解析info中房间信息 **auto room_info = info->get_map()["room"]**
8. 解析room_info中参会者信息 **auto users = room_info->get_map()["participants"]->get_vector()** 并触发TriggerOnUserJoined方法
9. 解析room_info中流信息 **auto streams = room_info->get_map()["streams"]->get_vector()** 并触发TriggerOnStreamAdded方法
10. 这里先做保留待分析了TriggerOnUserJoined及TriggerOnStreamAdded方法以后再给出结论

这里继续分析TriggerOnUserJoined及TriggerOnStreamAdded

```cpp
void ConferenceClient::TriggerOnUserJoined(sio::message::ptr user_info,
                                           bool joining) {
  Participant* user_raw;
  if (ParseUser(user_info, &user_raw)) {
    std::shared_ptr<Participant> user(user_raw);
    current_conference_info_->AddParticipant(user);
    if (!joining) {
      const std::lock_guard<std::mutex> lock(observer_mutex_);
      for (auto its = observers_.begin(); its != observers_.end(); ++its) {
        auto& o = (*its).get();
        event_queue_->PostTask([&o, user] { o.OnParticipantJoined(user); });
      }
    }
  }
}
```

 主要操作为 **event_queue_->PostTask([&o, user] { o.OnParticipantJoined(user)** 即遍历所有observers_ 触发OnParticipantJoined方法
 这里obserers_仅有ConferenceClientObserverObjcImpl实例类
 追踪到该实现部分为

 ```Objc
 void ConferenceClientObserverObjcImpl::OnParticipantJoined(
    std::shared_ptr<owt::conference::Participant> user) {
  OWTConferenceParticipant* participant =
      [[OWTConferenceParticipant alloc] initWithNativeParticipant:user];
  if ([delegate_
          respondsToSelector:@selector(conferenceClient:didAddParticipant:)]) {
    [delegate_ conferenceClient:client_ didAddParticipant:participant];
  }
}
 ```

 到这里就比较清晰了 **TriggerOnUserJoined** 方法最终通过OWTConferenceClientDelegate回调出去

 ```Objc
 -(void)conferenceClient:(OWTConferenceClient*)client didAddParticipan(OWTConferenceParticipant*)user;
 ```

那么猜想**TriggerOnStreamAdded**也会最终回调到onAddStream

下面开始分析验证思路

```cpp
void ConferenceClient::TriggerOnStreamAdded(sio::message::ptr stream_info,
                                            bool joining) {
  ParseStreamInfo(stream_info, joining);
}
```

该方法直接调用的 **ParseStreamInfo** 名为解析流信息,由于该方法太长(近400行代码)这里贴出主要的关键代码片段，如有对源码感兴趣的同学可自行查看对应方法的实现，这里就不贴出了

```cpp
const std::lock_guard<std::mutex> lock(stream_update_observer_mutex_);
        current_conference_info_->AddOrUpdateStream(remote_stream, updated);
if (!joining && !updated) {
    for (auto its = observers_.begin(); its != observers_.end(); ++its) {
        auto& o = (*its).get();
        event_queue_->PostTask(
           [&o, remote_stream] { o.OnStreamAdded(remote_stream); });
    }
}

```
和TriggerOnUserJoined一样通过observers_触发回调
这里需要注意一个信息是conferenceClient的成员变量current_conference_info_保存了流信息 **current_conference_info_->AddOrUpdateStream(remote_stream, updated)**

```Objc
void ConferenceClientObserverObjcImpl::OnStreamAdded(
    std::shared_ptr<RemoteMixedStream> stream) {
  OWTRemoteMixedStream* remote_stream =
      [[OWTRemoteMixedStream alloc] initWithNativeStream:stream];
  // Video formats
  /*
  NSMutableArray* supportedVideoFormats = [[NSMutableArray alloc] init];
  auto formats = stream->SupportedVideoFormats();
  for (auto it = formats.begin(); it != formats.end(); ++it) {
    RTCVideoFormat* format =
        [[RTCVideoFormat alloc] initWithNativeVideoFormat:(*it)];
    [supportedVideoFormats addObject:format];
  }
  [remote_stream setSupportedVideoFormats:supportedVideoFormats];*/
  AddRemoteStreamToMap(stream->Id(), remote_stream);
  if ([delegate_
          respondsToSelector:@selector(conferenceClient:didAddStream:)]) {
    [delegate_ conferenceClient:client_
                   didAddStream:(OWTRemoteStream*)remote_stream];
  }
}
```
对应的触发部分确实最终回调了didAddStream

至此第十步
10. OWTConferenceClient回调 **didAddParticipant** 及 **didAddStream **


第五步之前还未深入分析
接下来再继续深入第五步 目标很明确 signaling_channel_调用connect输入参数为token回调为info

之前有提到signaling_channel_是ConferenceSocketSignalingChannel构造出的实例
现在开始进入ConferenceSocketSignalingChannel实现部分

Connect方法也是过长（近300行代码） 这里就贴出主要的实现部分

1. 对token进行base64解码再从结果json中获取host
```cpp
rtc::GetStringFromJsonObject(json_token, "host", &host);
```

2. 初始化socket_client并设置参数及监听器等(这里socket库选用的是socket.io c++版本,其头文件为sio_client.h)
```
  socket_client_->socket(); //初始化socket
  socket_client_->set_reconnect_attempts(kReconnectionAttempts); //设置参 数(重连次数、重连时间间隔等等)
  socket_client_->set_reconnect_delay(kReconnectionDelay);
  socket_client_->set_reconnect_delay_max(kReconnectionDelayMax);
  socket_client_->set_socket_close_listener( //设置socket关闭监听器
      [weak_this](std::string const& nsp) {
        RTC_LOG(LS_INFO) << "Socket.IO disconnected.";
        auto that = weak_this.lock();
        if (that && (that->reconnection_attempted_ >= kReconnectionAttempts ||
                     that->disconnect_complete_)) {
          that->TriggerOnServerDisconnected();
        }
      });
  socket_client_->set_fail_listener([weak_this]() { //设置socket失败监听器
    RTC_LOG(LS_ERROR) << "Socket.IO connection failed.";
    auto that = weak_this.lock();
    if (that) {
      that->DropQueuedMessages();
      if (that->reconnection_attempted_ >= kReconnectionAttempts) {
        that->TriggerOnServerDisconnected();
      }
    }
  });
  socket_client_->set_reconnecting_listener([weak_this]() { // 设置socket重连监听器
    RTC_LOG(LS_INFO) << "Socket.IO reconnecting.";
    auto that = weak_this.lock();
    if (that) {
      if (that->reconnection_ticket_ != "") {
        // It will be reset when a reconnection is success (open listener) or
        // fail (fail listener).
        that->is_reconnection_ = true;
        that->reconnection_attempted_++;
      }
    }
  });
```

3. 主要内容在socket开启的监听器回调中
```cpp
socket_client_->set_open_listener([=](void) {
    // At this time the connect failure callback is still in pending list. No
    // need to add a new entry in the pending list.
    if (!is_reconnection_) {
      owt::base::SysInfo sys_info(owt::base::SysInfo::GetInstance());
      sio::message::ptr login_message = sio::object_message::create();
      login_message->get_map()["token"] = sio::string_message::create(token);
      sio::message::ptr ua_message = sio::object_message::create();
      sio::message::ptr sdk_message = sio::object_message::create();
      sdk_message->get_map()["type"] =
          sio::string_message::create(sys_info.sdk.type);
      sdk_message->get_map()["version"] =
          sio::string_message::create(sys_info.sdk.version);
      ua_message->get_map()["sdk"] = sdk_message;
      sio::message::ptr os_message = sio::object_message::create();
      os_message->get_map()["name"] =
          sio::string_message::create(sys_info.os.name);
      os_message->get_map()["version"] =
          sio::string_message::create(sys_info.os.version);
      ua_message->get_map()["os"] = os_message;
      sio::message::ptr runtime_message = sio::object_message::create();
      runtime_message->get_map()["name"] =
          sio::string_message::create(sys_info.runtime.name);
      runtime_message->get_map()["version"] =
          sio::string_message::create(sys_info.runtime.version);
      ua_message->get_map()["runtime"] = runtime_message;
      login_message->get_map()["userAgent"] = ua_message;
      std::string protocol_version = SIGNALING_PROTOCOL_VERSION;
      login_message->get_map()["protocol"] = sio::string_message::create(protocol_version);
      Emit("login", login_message,
           [=](sio::message::list const& msg) {
             connect_failure_callback_ = nullptr;
             if (msg.size() < 2) {
               RTC_LOG(LS_ERROR) << "Received unknown message while sending token.";
               if (on_failure != nullptr) {
                 std::unique_ptr<Exception> e(new Exception(
                     ExceptionType::kConferenceInvalidParam,
                     "Received unknown message from server."));
                 on_failure(std::move(e));
               }
               return;
             }
             sio::message::ptr ack =
                 msg.at(0);  // The first element indicates the state.
             std::string state = ack->get_string();
             if (state == "error" || state == "timeout") {
               RTC_LOG(LS_ERROR) << "Server returns " << state
                             << " while joining a conference.";
               if (on_failure != nullptr) {
                 std::unique_ptr<Exception> e(new Exception(
                     ExceptionType::kConferenceInvalidParam,
                     "Received error message from server."));
                 on_failure(std::move(e));
               }
               return;
             }
             // in signaling protocol 1.0.0, the response contains following info:
             // {id: string(participantid),
             //  user: string(userid),
             //  role: string(participantrole),
             //  permission: object(permission),
             //  room: object(RoomInfo),
             //  reconnectonTicket: undefined or string(ReconnecionTicket).}
             // At present client SDK will only save reconnection ticket and participantid
             // and ignoring other info.
             sio::message::ptr message = msg.at(1);

        // get room id .
        auto room_info = message->get_map()["room"];
        if (room_info == nullptr ||
            room_info->get_flag() != sio::message::flag_object) {
          RTC_DCHECK(false);
          return;
        }
        if (room_info->get_map()["id"]->get_flag() !=
            sio::message::flag_string) {
          RTC_DCHECK(false);
          return;
        } else {
            room_id_ = room_info->get_map()["id"]->get_string();
        }
        
        // get reconnection_ticket
             auto reconnection_ticket_ptr =
                 message->get_map()["reconnectionTicket"];
             if (reconnection_ticket_ptr) {
               OnReconnectionTicket(reconnection_ticket_ptr->get_string());
             }
             auto participant_id_ptr = message->get_map()["id"];
             if (participant_id_ptr) {
               participant_id_ = participant_id_ptr->get_string();
             }
             if (on_success != nullptr) {
               on_success(message);
             }
           },
           on_failure);
      is_reconnection_ = false;
      reconnection_attempted_ = 0;
    } else {
      socket_client_->socket()->emit(
          kEventNameRelogin, reconnection_ticket_, [&](sio::message::list const& msg) {
            if (msg.size() < 2) {
              RTC_LOG(LS_WARNING)
                  << "Received unknown message while reconnection ticket.";
              reconnection_attempted_ = kReconnectionAttempts;
              socket_client_->close();
              return;
            }
            sio::message::ptr ack =
                msg.at(0);  // The first element indicates the state.
            std::string state = ack->get_string();
            if (state == "error" || state == "timeout") {
              RTC_LOG(LS_WARNING)
                  << "Server returns " << state
                  << " when relogin. Maybe an invalid reconnection ticket.";
              reconnection_attempted_ = kReconnectionAttempts;
              socket_client_->close();
              return;
            }

            if (message->get_flag() == sio::message::flag_string) {
              OnReconnectionTicket(message->get_string());
            }
            DrainQueuedMessages();
          });
    }
  });
```

分析下来就是围绕 **Emit("login", login_message,[=](sio::message::list const& msg) {}** 在进行
1. emit参数为信令"login", 参数列表为login_message,回调为msg
2. login_message就是连接信令服务器所需参数是和信令服务器统一规定协商后的结果，实际企业项目中不一定是按照该参数配置，只需满足client验证及私密性安全性要求即可。
3. **[=](sio::message::list const& msg) {}** 回调部分取出了reconnectionTicket这个代表重连票据也就是信令重连恢复时需要传入的参数，表示该恢复会话，另外    **on_success(message)** 几乎回调了原始msg (见 **sio::message::ptr message = msg.at(1)** msg数组0为ack的状态信息)
4. 重连部分为else部分内容emit信令kEventNameRelogin，参数为reconnection_ticket_重连票据，然后做了timeout和error情况的处理，然后刷新重连票据等操作。

到这里也比较清晰加入媒体房间的信令流程，总的来说就是通过socket.io进行的信令交互client login的时候信令服务器进行数据及授权验证，返回房间信息，client拿到房间信息并进行解析回调给上层业务



由于篇幅过长剩下的流程会单独开中篇及下篇来分析


### 珍惜他人劳动成果，转载请注明[原文地址](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/OpenWebrtcToolkit%E6%BA%90%E7%A0%81%E8%A7%A3%E6%9E%90%E4%B8%8A%E7%AF%87.md)



 








