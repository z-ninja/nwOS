#ifndef NW_BYTE_SWAP_H
#define NW_BYTE_SWAP_H
#include <stdint.h>
uint16_t nw_swap_uint16( uint16_t val);
int16_t nw_swap_int16( int16_t val);
uint32_t nw_swap_uint32( uint32_t val);
int32_t nw_swap_int32( int32_t val);
int64_t nw_swap_int64( int64_t val);
uint64_t nw_swap_uint64( uint64_t val);
float nw_swap_float(float val);
double nw_swap_double(double val);

uint16_t nw_readUint16LE( uint16_t val );
uint16_t nw_writeUint16LE( uint16_t val );
uint16_t nw_readUint16BE( uint16_t val );
uint16_t nw_writeUint16BE( uint16_t val );


int16_t nw_readInt16LE( int16_t val );
int16_t nw_writeInt16LE( int16_t val );
int16_t nw_readInt16BE( int16_t val );
int16_t nw_writeInt16BE( int16_t val );

uint32_t nw_readUint32LE( uint32_t val );
uint32_t nw_writeUint32LE( uint32_t val );
uint32_t nw_readUint32BE( uint32_t val );
uint32_t nw_writeUint32BE( uint32_t val );


int32_t nw_readInt32LE( int32_t val );
int32_t nw_writeInt32LE( int32_t val );
int32_t nw_readInt32BE( int32_t val );
int32_t nw_writeInt32BE( int32_t val );


uint64_t nw_readUint64LE( uint64_t val );
uint64_t nw_writeUint64LE( uint64_t val );
uint64_t nw_readUint64BE( uint64_t val );
uint64_t nw_writeUint64BE( uint64_t val );

int64_t nw_readInt64LE( int64_t val );
int64_t nw_writeInt64LE( int64_t val );
int64_t nw_readInt64BE( int64_t val );
int64_t nw_writeInt64BE( int64_t val );


float nw_readFloatLE( float val );
float nw_writeFloatLE( float val );
float nw_readFloatBE( float val );
float nw_writeFloatBE( float val );


double nw_readDoubleLE( double val );
double nw_writeDoubleLE( double val );
double nw_readDoubleBE( double val );
double nw_writeDoubleBE( double val );
#endif // NW_BYTE_SWAP_H
