generate_protobuf(PROTO_SRCS PROTO_HDRS tivs/audio/audio.proto)
GENERATE_SERVICE_SEPARATE(SERVICE_SRCS SERVICE_HDRS tivs/audio/audio_service.proto)
GENERATE_CONFIG(SERVICE_CONF_SRC SERVICE_CONF_HDR tivs/audio/AudioServiceConfig.proto)

AUX_SOURCE_DIRECTORY(tivs/audio AUDIO_SRCS)
cxx_executable_no_link(AudioServer tivs/audio ${AUDIO_SRCS} ${PROTO_SRCS} ${SERVICE_SRCS} ${SERVICE_CONF_SRC})
add_definitions(-DUSE_LITTLE_ENDIAN)
cxx_link_craft(AudioServer ${COMMON_PROJECT_DEPENDENCIES})

install(TARGETS AudioServer RUNTIME DESTINATION /opt/tivs-audio/bin)
install(FILES ${PROJECT_SOURCE_DIR}/doc/AudioServer.conf DESTINATION /opt/tivs-audio/etc)
