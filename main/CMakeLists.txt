idf_component_register(SRCS  "main.cpp" 
                             "lib/tool/tool.cpp"
                             "lib/functions/functions.cpp"
                             "lib/udp_server.cpp"
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES ca_cert.pem)
