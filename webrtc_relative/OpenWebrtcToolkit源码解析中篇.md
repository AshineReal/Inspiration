
# Open webrtc toolkit源码解析(中篇)

### 珍惜他人劳动成果，转载请注明[原文地址](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/OpenWebrtcToolkit%E6%BA%90%E7%A0%81%E8%A7%A3%E6%9E%90%E4%B8%8A%E7%AF%87.md)

#### 继续[上篇](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/OpenWebrtcToolkit%E6%BA%90%E7%A0%81%E8%A7%A3%E6%9E%90%E4%B8%8A%E7%AF%87.md)的内容，这里回顾一下，上篇主要分析了配置client及入会流程，入会流程中通过媒体信令通道socket.io建立tcp连接，然后获取到房间信息，解析并缓存，通过匿名函数回调到join调用处。也就是说此时join的回调已经获取到了房间info，业务方可以对其进行选择订阅或者不订阅，业务方此时也可以选择发布或者不发布自己的流

#### 这里有个概念就是，owt遵循更加自由灵活度更高的方式，让业务方自行选择并决定订阅流和发布流的策略，没有进行自动订阅（这里发布和订阅的概念和licode中使用的概念类似）。所以想要看到房间内其它人的流，需要对该流进行订阅操作，只有发布过的流才会存在在房间信息内的流列表中，对应的其它端若是取消发布流，则流列表也会去掉该流（owt对外接口没有暴露取消订阅或取消发布的api，内部实现和licode类似是有这个概念和操作的），所以这是一个动态过程，不管用户何时加入房间都可以获取到最新的流列表，接下来就来分析对于已经加入房间中的用户，如何实时获取到新的流变化并作出何处理流程

### 基于事件监听的socket.io模型

socket.io发送事件使用emit方法

```cpp
void emit(std::string const& name, message::list const& msglist = nullptr, std::function<void (message::list const&)> const& ack = nullptr);

```

socket.io监听事件使用on方法

```cpp
void on(std::string const& event_name,event_listener const& func);

void on(std::string const& event_name,event_listener_aux const& func);
```

提一下 这里event_listener和event_listener_aux定义是通过c++函数模版

```cpp
typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::list& ack_message)> event_listener_aux;

typedef std::function<void(event& event)> event_listener;
```

emit在上篇中已经提到login及relogin操作
这里来看下源码中基于事件监听的方法回调
在conferencesocketsignalingchannnel实现connect建立socket连接的方法中

```cpp
.
..
...

for (const std::string& notification_name :
       {kEventNameStreamMessage, kEventNameTextMessage,
        kEventNameOnUserPresence, kEventNameOnSignalingMessage,
        kEventNameOnDrop}) {
    socket_client_->socket()->on(
        notification_name,
        sio::socket::event_listener_aux(std::bind(
            &ConferenceSocketSignalingChannel::OnNotificationFromServer, this,
            std::placeholders::_1, std::placeholders::_2))); // 这里转接到方法OnNotificationFromServer
  }

  socket_client_->socket()->on(
      kEventNameConnectionFailed,
      sio::socket::event_listener_aux(
          [&](std::string const& name, sio::message::ptr const& data,
              bool is_ack, sio::message::list& ack_resp) {
            for (auto it = observers_.begin(); it != observers_.end(); ++it) {
              (*it)->OnStreamError(data);
            }
        }));

```

这里主要看正常的监听 notification_name 调用方法 OnNotificationFromServer

继续往下走 该方法较长(近100行)因为该方法对于分析过程比较重要，这里还是贴出全部代码

```cpp
void ConferenceSocketSignalingChannel::OnNotificationFromServer(
    const std::string& name,
    sio::message::ptr const& data) {
  if (name == kEventNameStreamMessage) {
    RTC_LOG(LS_VERBOSE) << "Received stream event.";
    if (data->get_map()["status"] != nullptr &&
        data->get_map()["status"]->get_flag() == sio::message::flag_string &&
        data->get_map()["id"] != nullptr &&
        data->get_map()["id"]->get_flag() == sio::message::flag_string) {
      std::string stream_status = data->get_map()["status"]->get_string();
      std::string stream_id = data->get_map()["id"]->get_string();
      if (stream_status == "add") {
        auto stream_info = data->get_map()["data"];
        if (stream_info != nullptr &&
            stream_info->get_flag() == sio::message::flag_object) {
          for (auto it = observers_.begin(); it != observers_.end(); ++it) {
            (*it)->OnStreamAdded(stream_info);
          }
        }
      } else if (stream_status == "update") {
        sio::message::ptr update_message = sio::object_message::create();
        update_message->get_map()["id"] =
            sio::string_message::create(stream_id);
        auto stream_update = data->get_map()["data"];
        if (stream_update != nullptr &&
            stream_update->get_flag() == sio::message::flag_object) {
          update_message->get_map()["event"] = stream_update;
        }
        for (auto it = observers_.begin(); it != observers_.end(); ++it) {
          (*it)->OnStreamUpdated(update_message);
        }
      } else if (stream_status == "remove") {
        sio::message::ptr remove_message = sio::object_message::create();
        remove_message->get_map()["id"] =
            sio::string_message::create(stream_id);
        for (auto it = observers_.begin(); it != observers_.end(); ++it) {
          (*it)->OnStreamRemoved(remove_message);
        }
      }
    }
  } else if (name == kEventNameTextMessage) {
    RTC_LOG(LS_VERBOSE) << "Received custom message.";
    std::string from = data->get_map()["from"]->get_string();
    std::string message = data->get_map()["message"]->get_string();
    std::string to = "me";
    auto target = data->get_map()["to"];
    if (target != nullptr && target->get_flag() == sio::message::flag_string) {
      to = target->get_string();
    }
    for (auto it = observers_.begin(); it != observers_.end(); ++it) {
      (*it)->OnCustomMessage(from, message, to);
    }
  } else if (name == kEventNameOnUserPresence) {
    RTC_LOG(LS_VERBOSE) << "Received user join/leave message.";
    if (data == nullptr || data->get_flag() != sio::message::flag_object ||
        data->get_map()["action"] == nullptr ||
        data->get_map()["action"]->get_flag() != sio::message::flag_string) {
      RTC_DCHECK(false);
      return;
    }
    auto participant_action = data->get_map()["action"]->get_string();
    if (participant_action == "join") {
      // Get the pariticipant ID from data;
      auto participant_info = data->get_map()["data"];
      if (participant_info != nullptr &&
          participant_info->get_flag() == sio::message::flag_object &&
          participant_info->get_map()["id"] != nullptr &&
          participant_info->get_map()["id"]->get_flag() ==
              sio::message::flag_string) {
        for (auto it = observers_.begin(); it != observers_.end(); ++it) {
          (*it)->OnUserJoined(participant_info);
        }
      }
    } else if (participant_action == "leave") {
      auto participant_info = data->get_map()["data"];
      if (participant_info != nullptr &&
          participant_info->get_flag() == sio::message::flag_string) {
        for (auto it = observers_.begin(); it != observers_.end(); ++it) {
          (*it)->OnUserLeft(participant_info);
        }
      }
    } else {
      RTC_NOTREACHED();
    }
  } else if (name == kEventNameOnSignalingMessage) {
    RTC_LOG(LS_VERBOSE) << "Received signaling message from erizo.";
    for (auto it = observers_.begin(); it != observers_.end(); ++it) {
      (*it)->OnSignalingMessage(data);
    }
  } else if (name == kEventNameOnDrop) {
    RTC_LOG(LS_INFO) << "Received drop message.";
    socket_client_->set_reconnect_attempts(0);
    for (auto it = observers_.begin(); it != observers_.end(); ++it) {
      (*it)->OnServerDisconnected();
    }
  }
}
```

分析下来就是几个event的case

1. kEventNameStreamMessage这个是定义的关于流的一些信息的事件
2. kEventNameTextMessage这个是定义的关于文本消息的一些信息的事件(没错 这里提到了文本消息的发送，owt提供了简单的本文消息发送，丰富了基于webrtc音视频会议房间的业务场景)
3. kEventNameOnUserPresence定义了关于用户进退及用户信息更新的信息事件
4. kEventNameOnSignalingMessage定义了信令消息(先揭露之后再详细说明 这里其实是sdp协商过程所需的offer answer candidate等信令协商事件)
5. kEventNameOnDrop消息丢失服务器关闭连接

这里主要来看kEventNameStreamMessage事件

```cpp
std::string stream_status = data->get_map()["status"]
```

通过取出的status又区分了add、update、remove的状态
分别通过监听者触发OnStreamAdded、OnStreamUpdated、OnStreamRemoved

```cpp
for (auto it = observers_.begin(); it != observers_.end(); ++it) {
    (*it)->OnStreamAdded(stream_info);
}
```

```cpp
for (auto it = observers_.begin(); it != observers_.end(); ++it) {
    (*it)->OnStreamUpdated(update_message);
}
```

```cpp
for (auto it = observers_.begin(); it != observers_.end(); ++it) {
    (*it)->OnStreamRemoved(remove_message);
}
```

追踪到conferenceclient监听实现部分

```cpp
void ConferenceClient::OnStreamAdded(sio::message::ptr stream) {
  TriggerOnStreamAdded(stream);
}
```

// 到这里已经是不是觉得似曾相识，没错 join加入房间那里也触发了该方法都是解析流信息做解析缓存回调到业务层监听

```cpp
void ConferenceClient::TriggerOnStreamAdded(sio::message::ptr stream_info,bool joining) {
  ParseStreamInfo(stream_info, joining);
}
```

同理 **OnStreamUpdated** 和 **OnStreamRemoved** 就不贴出来了

### 总结: 基于事件监听的socket.io模型 通过事件监听回调流的增删改信息，这也解释了开头提出的问题，对于已经在会议中的用户，如何获取最新的流变更信息。

#### 文章开头已经提出了订阅和发布的概念这里继续分析这两个api


### Publish的流程

```Objc
- (void)publish:(OWTLocalStream*)stream
    withOptions:(nullable OWTPublishOptions*)options
      onSuccess:(nullable void (^)(OWTConferencePublication*))onSuccess
      onFailure:(nullable void (^)(NSError*))onFailure;
```

实现部分

```Objc
- (void)publish:(OWTLocalStream*)stream
    withOptions:(OWTPublishOptions*)options
      onSuccess:(void (^)(OWTConferencePublication*))onSuccess
      onFailure:(void (^)(NSError*))onFailure {
  RTC_CHECK(stream);
  auto nativeStreamRefPtr = [stream nativeStream];
  std::shared_ptr<owt::base::LocalStream> nativeStream(
      std::static_pointer_cast<owt::base::LocalStream>(nativeStreamRefPtr));
  if (options == nil) {
    _nativeConferenceClient->Publish( // conferenceclient调用publish方法
        nativeStream,
        [=](std::shared_ptr<owt::conference::ConferencePublication>
                publication) {
          [_publishedStreams setObject:stream forKey:[stream streamId]];
          if (onSuccess != nil)
            onSuccess([[OWTConferencePublication alloc]
                initWithNativePublication:publication]);
        },
        [=](std::unique_ptr<owt::base::Exception> e) {
          [self triggerOnFailure:onFailure withException:(std::move(e))];
        });
  } else {
    _nativeConferenceClient->Publish(
        nativeStream, *[options nativePublishOptions].get(),
        [=](std::shared_ptr<owt::conference::ConferencePublication>
                publication) {
          [_publishedStreams setObject:stream forKey:[stream streamId]];
          if (onSuccess != nil)
            onSuccess([[OWTConferencePublication alloc]
                initWithNativePublication:publication]);
        },
        [=](std::unique_ptr<owt::base::Exception> e) {
          [self triggerOnFailure:onFailure withException:(std::move(e))];
        });
  }
}
```

这里主要先分析publish的信令流程 对于webrtc层的处理留到下篇统一分析
继续追踪到conferenceclient调用publish处

```cpp
void ConferenceClient::Publish(
    std::shared_ptr<LocalStream> stream,
    const PublishOptions& options,
    std::function<void(std::shared_ptr<ConferencePublication>)> on_success,
    std::function<void(std::unique_ptr<Exception>)> on_failure) {
  if (!CheckNullPointer((uintptr_t)stream.get(), on_failure)) {
    RTC_LOG(LS_ERROR) << "Local stream cannot be nullptr.";
    return;
  }
  if (!CheckNullPointer((uintptr_t)(stream->MediaStream()), on_failure)) {
    RTC_LOG(LS_ERROR) << "Cannot publish a local stream without media stream.";
    return;
  }
  if (stream->MediaStream()->GetAudioTracks().size() == 0 &&
      stream->MediaStream()->GetVideoTracks().size() == 0) {
    RTC_LOG(LS_ERROR) << "Cannot publish a local stream without audio & video";
    std::string failure_message(
        "Publishing local stream with neither audio nor video.");
    if (on_failure != nullptr) {
      event_queue_->PostTask([on_failure, failure_message]() {
        std::unique_ptr<Exception> e(
            new Exception(ExceptionType::kConferenceUnknown, failure_message));
        on_failure(std::move(e));
      });
    }
    return;
  }
  if (!CheckSignalingChannelOnline(on_failure)) {
    RTC_LOG(LS_ERROR) << "Signaling channel disconnected.";
    return;
  }
  // Reorder SDP according to perference list.
  PeerConnectionChannelConfiguration config =
      GetPeerConnectionChannelConfiguration();
  for (auto codec : options.video) {
    config.video.push_back(VideoEncodingParameters(codec));
  }
  for (auto codec : options.audio) {
    config.audio.push_back(AudioEncodingParameters(codec));
  }
  std::shared_ptr<ConferencePeerConnectionChannel> pcc(
      new ConferencePeerConnectionChannel(config, signaling_channel_,
                                          event_queue_));
  pcc->AddObserver(*this);
  {
    std::lock_guard<std::mutex> lock(publish_pcs_mutex_);
    publish_pcs_.push_back(pcc);
  }
  std::weak_ptr<ConferenceClient> weak_this = shared_from_this();
  std::string stream_id = stream->Id();
  pcc->Publish(stream,
               [on_success, weak_this, stream_id](std::string session_id) {
                 auto that = weak_this.lock();
                 if (!that)
                   return;
                 // map current pcc
                 if (on_success != nullptr) {
                   std::shared_ptr<ConferencePublication> cp(
                       new ConferencePublication(that, session_id, stream_id));
                   on_success(cp);
                 }
               },
               on_failure);
}
```

通过一系列的异常处理及状态过滤来到 **pcc->publish** 而pcc是ConferencePeerConnectionChannel构造出的实例

```cpp
std::shared_ptr<ConferencePeerConnectionChannel> pcc(
      new ConferencePeerConnectionChannel(config, signaling_channel_,event_queue_));
```

继续追踪到ConferencePeerConnectionChannel的publish实现

```cpp
// Failure of publish will be handled here directly; while success needs
// conference client to construct the ConferencePublication instance,
// So we're not passing success callback here.
void ConferencePeerConnectionChannel::Publish(
    std::shared_ptr<LocalStream> stream,
    std::function<void(std::string)> on_success,
    std::function<void(std::unique_ptr<Exception>)> on_failure) {
  RTC_LOG(LS_INFO) << "Publish a local stream.";
  published_stream_ = stream;
  if ((!CheckNullPointer((uintptr_t)stream.get(), on_failure)) ||
      (!CheckNullPointer((uintptr_t)stream->MediaStream(), on_failure))) {
    RTC_LOG(LS_INFO) << "Local stream cannot be nullptr.";
  }
  if (IsMediaStreamEnded(stream->MediaStream())) {
    if (on_failure != nullptr) {
      event_queue_->PostTask([on_failure]() {
        std::unique_ptr<Exception> e(new Exception(
            ExceptionType::kConferenceUnknown, "Cannot publish ended stream."));
        on_failure(std::move(e));
      });
    }
    return;
  }
  publish_success_callback_ = on_success;
  failure_callback_ = on_failure;
  audio_transceiver_direction_=webrtc::RtpTransceiverDirection::kSendOnly;
  video_transceiver_direction_=webrtc::RtpTransceiverDirection::kSendOnly;
  sio::message::ptr options = sio::object_message::create();
  // attributes
  sio::message::ptr attributes_ptr = sio::object_message::create();
  for (auto const& attr : stream->Attributes()) {
    attributes_ptr->get_map()[attr.first] =
        sio::string_message::create(attr.second);
  }
  options->get_map()[kStreamOptionAttributesKey] = attributes_ptr;
  // media
  sio::message::ptr media_ptr = sio::object_message::create();
  if (stream->MediaStream()->GetAudioTracks().size() == 0) {
    media_ptr->get_map()["audio"] = sio::bool_message::create(false);
  } else {
    sio::message::ptr audio_options = sio::object_message::create();
    if (stream->Source().audio == owt::base::AudioSourceInfo::kScreenCast) {
      audio_options->get_map()["source"] =
          sio::string_message::create("screen-cast");
    } else {
      audio_options->get_map()["source"] = sio::string_message::create("mic");
    }
    media_ptr->get_map()["audio"] = audio_options;
  }
  if (stream->MediaStream()->GetVideoTracks().size() == 0) {
    media_ptr->get_map()["video"] = sio::bool_message::create(false);
  } else {
    sio::message::ptr video_options = sio::object_message::create();
    if (stream->Source().video == owt::base::VideoSourceInfo::kScreenCast) {
      video_options->get_map()["source"] =
          sio::string_message::create("screen-cast");
    } else {
      video_options->get_map()["source"] =
          sio::string_message::create("camera");
    }
    media_ptr->get_map()["video"] = video_options;
  }
  options->get_map()["media"] = media_ptr;
  SendPublishMessage(options, stream, on_failure);
}
```

首先是一些异常处理，然后主要是创建几个socket.io的消息

```cpp
// option
sio::message::ptr options = sio::object_message::create();
// attributes
sio::message::ptr attributes_ptr = sio::object_message::create();
// media set into option
sio::message::ptr media_ptr = sio::object_message::create();
...
..
.
SendPublishMessage(options, stream, on_failure);//最终调用这里
```

继续到SendPublishMessage实现中

```cpp
void ConferencePeerConnectionChannel::SendPublishMessage(
    sio::message::ptr options,
    std::shared_ptr<LocalStream> stream,
    std::function<void(std::unique_ptr<Exception>)> on_failure) {
  signaling_channel_->SendInitializationMessage(
      options, stream->MediaStream()->id(), "",
      [stream, this](std::string session_id) {
        SetSessionId(session_id);
        for (const auto track : stream->MediaStream()->GetAudioTracks()) {
          webrtc::RtpTransceiverInit transceiver_init;
          transceiver_init.stream_ids.push_back(stream->MediaStream()->id());
          transceiver_init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
          AddTransceiver(track, transceiver_init);
        }
        for (const auto track : stream->MediaStream()->GetVideoTracks()) {
          webrtc::RtpTransceiverInit transceiver_init;
          transceiver_init.stream_ids.push_back(stream->MediaStream()->id());
          transceiver_init.direction =
              webrtc::RtpTransceiverDirection::kSendOnly;
          if (configuration_.video.size() > 0 &&
              configuration_.video[0].rtp_encoding_parameters.size() != 0) {
            for (auto encoding :
                 configuration_.video[0].rtp_encoding_parameters) {
              webrtc::RtpEncodingParameters param;
              if (encoding.rid != "")
                param.rid = encoding.rid;
              if (encoding.max_bitrate_bps != 0)
                param.max_bitrate_bps = encoding.max_bitrate_bps;
              if (encoding.max_framerate != 0)
                param.max_framerate = encoding.max_framerate;
              if (encoding.scale_resolution_down_by > 0)
                param.scale_resolution_down_by =
                    encoding.scale_resolution_down_by;
              if (encoding.num_temporal_layers > 0 &&
                  encoding.num_temporal_layers <= 4) {
                param.num_temporal_layers = encoding.num_temporal_layers;
              }
              param.active = encoding.active;
              transceiver_init.send_encodings.push_back(param);
            }
          }
          AddTransceiver(track, transceiver_init);
        }
        CreateOffer();
      },
      on_failure);
}
```

这个方法其实只是包装了 **signaling_channel_->SendInitializationMessage(options,publish_stream_label,subscribe_stream_label,on_success,on_failure)** 方法的实现(这里on_success的回调部分涉及到webrtc处理部分下篇来讲)
继续往下分析

```cpp
void ConferenceSocketSignalingChannel::SendInitializationMessage(
    sio::message::ptr options,
    std::string publish_stream_label,
    std::string subscribe_stream_label,
    std::function<void(std::string)> on_success,
    std::function<void(std::unique_ptr<Exception>)> on_failure) {
  sio::message::list message_list;
  message_list.push(options);
  std::string event_name;
  if (publish_stream_label != "")
    event_name = kEventNamePublish;
  else if (subscribe_stream_label != "")
    event_name = kEventNameSubscribe;
  Emit(event_name, message_list,
       [=](sio::message::list const& msg) {
         RTC_LOG(LS_INFO) << "Received ack from server.";
         if (on_success == nullptr) {
           RTC_LOG(LS_WARNING) << "Does not implement success callback. Make sure "
                              "it is what you want.";
           return;
         }
         sio::message::ptr message = msg.at(0);
         if (message->get_flag() != sio::message::flag_string) {
           RTC_LOG(LS_WARNING)
               << "The first element of publish ack is not a string.";
           if (on_failure) {
             std::unique_ptr<Exception> e(new Exception(
                 ExceptionType::kConferenceInvalidParam,
                 "Received unkown message from server."));
             on_failure(std::move(e));
           }
           return;
         }
         if (message->get_string() == "ok") {
           if (msg.at(1)->get_flag() != sio::message::flag_object) {
             RTC_DCHECK(false);
             return;
           }
           std::string session_id = msg.at(1)->get_map()["id"]->get_string();
           if (event_name == kEventNamePublish || event_name == kEventNameSubscribe) {
             // Notify PeerConnectionChannel.
             on_success(session_id);
             return;
           }
           return;
         } else if (message->get_string() == "error" && msg.at(1) != nullptr &&
                    msg.at(1)->get_flag() == sio::message::flag_string) {
           if (on_failure) {
             std::unique_ptr<Exception> e(new Exception(
                 ExceptionType::kConferenceNotSupported, msg.at(1)->get_string()));
             on_failure(std::move(e));
           }
         } else {
           if (on_failure) {
             std::unique_ptr<Exception> e(new Exception(
                 ExceptionType::kConferenceInvalidParam,
                 "Ack for initializing message is not expected."));
             on_failure(std::move(e));
           }
           return;
         }
       },
       on_failure);
}
```

通过 **event_name = kEventNamePublish** 及 **event_name = kEventNameSubscribe** 看出publish和subscribe共享了该方法
emit发送的参数为event_name及options,回应ack message为“ok”时回调on_success
注意! 这里当ack message的status为ok时 取出了session_id **std::string session_id = msg.at(1)->get_map()["id"]** 并把session_id通过on_success回调回去（记住这个session_id, 原因之后解释,这里先给出结论:这个session_id才是publish发布流的关键,它相当于在服务器端创建了一个会话标识,后续的流的sdp交互等流相关操作都会通过该标识作为关键key传递）

```cpp
void ConferencePeerConnectionChannel::SetSessionId(const std::string& id) {
  RTC_LOG(LS_INFO) << "Setting session ID for current channel";
  session_id_ = id;
}
std::string ConferencePeerConnectionChannel::GetSessionId() const {
  return session_id_;
}
```

再回来看conferencepeerconnectionchannel的on_success部分将返回的session_id的
进行了成员变量保存并提供了set和get的方式(这里使用成员变量是因为pcc的实例是每次publish都会构建新的，也就是说两次publish之间是独立不影响的，这里先给出结论subscribe也是同理，也就是说conferenceclient依靠conferencepeerconnectionchannel做到了不同publish不同subscribe之间的业务隔离，conferencepeerconnectionchannel只需关注当前成员变量session_id_描述的流维护即可)

至此publish发布流的过程已经清晰
OWTConferenceClient -> conferenceclient -> conferencepeerconnectionchannel -> conferencesocketsignalingchannel
-> socketio

ack的流程则与之相反
socketio -> conferencesocketsignalingchannel -> conferencepeerconnectionchannel
而webrtc处理主要集中在conferencepeerconnectionchannel，conferencesocketsignalingchannel则是集中处理和接收socketio的消息

### Subscribe的流程

由于publish的流程和publish类似这里只贴出关键代码

conferenceclient中subscribe片段

```cpp
 pcc->Subscribe(
      stream, options,
      [on_success, weak_this, stream_id](std::string session_id) {
        auto that = weak_this.lock();
        if (!that)
          return;
        // map current pcc
        if (on_success != nullptr) {
          std::shared_ptr<ConferenceSubscription> cp(
              new ConferenceSubscription(that, session_id, stream_id));
          on_success(cp);
        }
      },
      on_failure);
```

conferencepeerconnectionchannel中片段

```cpp
 signaling_channel_->SendInitializationMessage(
      sio_options, "", stream->Id(),
      [this](std::string session_id) {
        // Pre-set the session's ID.
        SetSessionId(session_id);
        CreateOffer();
      },
      on_failure);  // TODO: on_failure
```

可以看到最终都是调用SendInitializationMessage方法回调session_id
传入的streamId并没有使用仅当作publish还是subscribe的占位判断所以实际标识还是使用的session_id

#### 总的来看owt信令实现了逻辑分层,conferenceclient作为最初的入口及回调最后的出口基本管控OWTConferenceClient暴露出来的接口,conferencepeerconnectionchannel管控粒度更加细致的工作，包括发布订阅流及sdp生成接收安装等主要依赖webrtc的实现，接下来就是conferencesocketsignalingchannel实现对接socketio，提供了各上层需要的信令交互最基本的api调用实现和事件监听分流回调，总结如下图所示(由于仅分析了源码部分conference模块，p2p的实现与此类似，就不多作陈述，图示也不具体给出了)

[owt_level](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/image_bed/owt_level)


#### 至此上中篇已经分析了owt中信令的交互流程下篇我们再详细分析其中webrtc建立流连接的过程

### 珍惜他人劳动成果，转载请注明[原文地址](https://github.com/AshineReal/Inspiration/blob/master/webrtc_relative/OpenWebrtcToolkit%E6%BA%90%E7%A0%81%E8%A7%A3%E6%9E%90%E4%B8%8A%E7%AF%87.md)




























































