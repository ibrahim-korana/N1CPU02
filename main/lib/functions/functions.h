
#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include "core.h"
#include "storage.h"
#include "i2cdev.h"
#include "classes.h"
#include "termostat.h"

//bool out_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin);
//bool in_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin);

   Base_Function *find_register(uint8_t sender, uint8_t rid);
   Base_Function *function_find(uint8_t id);
   
   uint8_t functions_remote_register(const char *name, 
                                  const char *auname,
                                  uint8_t sender, 
                                  uint8_t rid, 
                                  uint8_t prg_lc,
                                  transmisyon_t yer,
                                  function_callback_t fun_cb,
                                  function_callback_t com_cb,
                                  Storage dsk
                                  ) ;
   uint8_t function_remote_re_register(
                                     function_reg_t *old,
                                     function_callback_t fun_cb,
                                     function_callback_t com_cb,
                                     Storage dsk
                                   );                               

   void *Read_functions(function_callback_t fun_cb, //fonksiyon eventleri için callback 
                        register_callback_t reg_cb, //registerler için callback
                        Storage dsk,
                        i2c_dev_t **pcf0);
   void function_list(void); 

   Base_Function *get_function_head_handle(void);  
   prg_location_t *get_location_head_handle(void);  
   Termostat *get_termostat_head_handle(void);   
   void set_function_head_handle(Base_Function * bas);
   void add_locations(prg_location_t *lc);
   void list_locations(void); 
   void *read_locations(Storage dsk);   
   uint8_t function_count(void); 
   void *read_gateway(Storage dsk, rs485_callback_t cb);
   Base_Function *function_find(uint8_t id);


#endif