# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "1.1.1.14")
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
# 1.1.1.2  --> 4.3.2024
    # mwater başlangıcı ile ilgili eklemeler yapıldı     
# 1.1.1.3  --> 11.3.2024
    # kontaktör başlangıcı ile ilgili eklemeler yapıldı   
# 1.1.1.4 ---> 20.10.2024
    # akil anahtar sisteme eklendi    
# 1.1.1.6 ---> 20.10.2024
    # Hard Reset sisteme eklendi    
# 1.1.1.7 ---> 31.12.2024
    # inportlara direction register eklendi. 
# 1.1.1.8 ---> 31.12.2024
    # i2c probe degiştirildi.   
# 1.1.1.9 ---> 06.01.2024
    # Curtain, Air, Lamba köklü degişimi. Yeni android ve akille entegrasyon      
# 1.1.1.10 ---> 07.01.2024
    # Garaj kapısı gara eklendi    
# 1.1.1.11 ---> 12.01.2024
    # Log sistemi eklendi   
# 1.1.1.12 ---> 14.01.2024
    # Log sistemi düzenlendi     
# 1.1.1.13 ---> 14.01.2024
    # TFTP OTA Server eklendi   
# 1.1.1.14 ---> 15.01.2024
    # Log sistemi web e baglandı Default kayıtsız                  

