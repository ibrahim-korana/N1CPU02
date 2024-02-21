# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.1.1.1")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(N1CPU02)

spiffs_create_partition_image(storage data FLASH_IN_PROJECT)


# 1.1.0.7 -->  8.12.2023 pistonda once motor 2sn sonra up/down motorunun calışması sağlandı.
# 1.1.0.9 -->  17.12.2023 extenden pcf pasiflestirildi
# 1.1.0.10
    # Ota Upgrade eklendi
    # Boot i2c kontrolünden önce geciktirildi. 
# 1.1.0.11
    # Açılışta I2C için deviceların tümünün kontrolünden sonra reset sağlandı.      
# 1.1.0.12
    # Test düzenlendi    
# 1.1.1.1
    # IDF 5.1.2 Major upgrade	