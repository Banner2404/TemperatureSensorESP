idf_component_register(SRCS "main.c"
                            "sensor.c"
                            "shadow.c"
                            "wifi.c"
                    INCLUDE_DIRS "")

target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate.pem.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private.pem.key" TEXT)