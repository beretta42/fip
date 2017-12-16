// Some words to send vt52 codes
: char str "lit" find compile word drop wordb 2 + c@ , ; imm
: esc 1b emit ;
: home esc char H emit ;
: clear esc char J emit ;
: cls home clear ;
: fcolor esc char b emit 30 + emit ; 
: bcolor esc char c emit 30 + emit ;
: goto esc char Y emit 20 + emit 20 + emit ;

