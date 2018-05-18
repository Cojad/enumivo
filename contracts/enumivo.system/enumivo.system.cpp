#include "enumivo.system.hpp"
#include <enumivolib/dispatcher.hpp>

#include "delegate_bandwidth.cpp"
#include "producer_pay.cpp"
#include "voting.cpp"
#include "exchange_state.cpp"


namespace enumivosystem {

   system_contract::system_contract( account_name s )
   :native(s),
    _voters(_self,_self),
    _producers(_self,_self),
    _global(_self,_self),
    _rammarket(_self,_self)
   {
      //print( "construct system\n" );
      _gstate = _global.exists() ? _global.get() : get_default_parameters();

      auto itr = _rammarket.find(S(4,RAMEOS));

      if( itr == _rammarket.end() ) {
         auto system_token_supply   = enumivo::token(N(enumivo.coin)).get_supply(enumivo::symbol_type(system_token_symbol).name()).amount;
         if( system_token_supply > 0 ) {
            itr = _rammarket.emplace( _self, [&]( auto& m ) {
               m.supply.amount = 100000000000000ll;
               m.supply.symbol = S(4,RAMEOS);
               m.base.balance.amount = int64_t(_gstate.free_ram());
               m.base.balance.symbol = S(0,RAM);
               m.quote.balance.amount = system_token_supply / 1000;
               m.quote.balance.symbol = S(4,EOS);
            });
         }
      } else {
         //print( "ram market already created" );
      }
   }

   enumivo_global_state system_contract::get_default_parameters() {
      enumivo_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }


   system_contract::~system_contract() {
      //print( "destruct system\n" );
      _global.set( _gstate, _self );
      //enumivo_exit(0);
   }

   void system_contract::setram( uint64_t max_ram_size ) {
      require_auth( _self );

      enumivo_assert( max_ram_size < 1024ll*1024*1024*1024*1024, "ram size is unrealistic" );
      enumivo_assert( max_ram_size > _gstate.total_ram_bytes_reserved, "attempt to set max below reserved" );

      auto delta = int64_t(max_ram_size) - int64_t(_gstate.max_ram_size);
      auto itr = _rammarket.find(S(4,RAMEOS));

      /**
       *  Increase or decrease the amount of ram for sale based upon the change in max
       *  ram size.
       */
      _rammarket.modify( itr, 0, [&]( auto& m ) {
         m.base.balance.amount += delta;
      });

      _gstate.max_ram_size = max_ram_size;
      _global.set( _gstate, _self );
   }

   void system_contract::setpriv( account_name account, uint8_t ispriv ) {
      require_auth( _self );
      set_privileged( account, ispriv );
   }

} /// enumivo.system
 

ENUMIVO_ABI( enumivosystem::system_contract,
     (setram)
     // delegate_bandwith.cpp
     (delegatebw)(undelegatebw)(refund)
     (buyram)(buyrambytes)(sellram)
     // voting.cpp
     // producer_pay.cpp
     (regproxy)(regproducer)(unregprod)(voteproducer)
     (claimrewards)
     // native.hpp
     (onblock)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(postrecovery)(passrecovery)(vetorecovery)(onerror)(canceldelay)
     //this file
     (setpriv)
)