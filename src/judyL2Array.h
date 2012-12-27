#ifndef JUDYL2ARRAY_H
#define JUDYL2ARRAY_H

///NOTE is this worth pursuing? might be better to use *std::vector for JudyValue...

/****************************************************************************//**
* \file judyL2Array.h C++ wrapper for judyL2 array implementation
*
*    A judyL2 array maps a set of ints to multiple ints, similar to std::multimap.
*    Internally, this is a judyL array of std::vector< JudyValue >.
*
*    Author: Mark Pictor. Public domain.
*
********************************************************************************/

#include "judy.h"
#include "assert.h"
// #include <string.h>
#include <vector>

template< typename JudyKey, typename JudyValue >
struct judyl2KVpair {
    JudyKey key;
    JudyValue value;
};

/**
 * integer keys are passed as arrays of native integers (32 or 64 bits) and the depth of the array is passed when the Judy array is created. The address of the high-order integer key pointer is passed to the judy functions as a character pointer and the string length argument should be set to the depth * JUDY_key_size.
 */

/** A judyL2 array maps a set of ints to multiple ints, similar to std::multimap.
 * Internally, this is a judyL array of std::vector< JudyValue >.
 * The first template parameter must be the same size as a void*
 *  \param JudyKey the type of the key, i.e. uint64_t, etc
 *  \param JudyValue the type of the value, i.e. int, pointer-to-object, etc. With judyL2Array, the size of this value can vary.
 */
template< typename JudyKey, typename JudyValue >
class judyL2Array {
    public:
        typedef std::vector< JudyValue > vector;
        typedef const std::vector< JudyValue > cvector;
        typedef judyl2KVpair< JudyKey, vector * > pair;
        typedef judyl2KVpair< JudyKey, cvector * > cpair;
protected:
        Judy * _judyarray;
        unsigned int _maxLevels, _depth;
        vector ** _lastSlot;
        JudyKey _buff[1];
        bool _success;
        //const int allocSize = sizeof( std::vector < JudyValue > ); //TODO store vectors inside judy with placement new?
    public:
        judyL2Array(): _maxLevels( 2 ^ ( sizeof( JudyKey ) + 2 ) ), _depth( 16 / JUDY_key_size ), _lastSlot( 0 ), _success( true ) {
            _judyarray = judy_open( _maxLevels, _depth );
            _buff[0] = 0;
            assert( sizeof( JudyKey ) == JUDY_key_size && "JudyKey *must* be the same size as a pointer!" );
        }

        explicit judyL2Array( const judyL2Array< JudyKey, JudyValue > & other ): _maxLevels( other._maxLevels ),
            _depth( other._depth ), _success( other._success ) {
            _judyarray = judy_clone( other._judyarray );
            _buff[0] = other._buff[0];
            find( _buff ); //set _lastSlot
        }

        ~judyL2Array() {
            //TODO delete all std::vectors first
            judy_close( _judyarray );
        }

        JudyValue getLastValue() {
            assert( _lastSlot );
            return &_lastSlot;
        }

        void setLastValue( JudyValue value ) {
            assert( _lastSlot );
            &_lastSlot = value;
        }

        bool success() {
            return _success;
        }

        //TODO
        // allocate data memory within judy array for external use.
        // void *judy_data (Judy *judy, unsigned int amt);

        /// insert value into the vector for key.
        void insert( JudyKey key, JudyValue value ) {
            _lastSlot = ( vector ** ) judy_cell( _judyarray, ( const unsigned char * ) &key, _depth * JUDY_key_size );
            if( ! ( * _lastSlot ) ) {
                * _lastSlot = new vector;
            }
            ( * _lastSlot )->push_back( value );
        }

        //TODO
        // for a given key, append to or replace the vector
        //void insert( JudyKey key, vector values, bool replace = false ) {}

        /// retrieve the cell pointer greater than or equal to given key
        /// NOTE what about an atOrBefore function?
        const pair atOrAfter( JudyKey key ) {
            _lastSlot = ( vector ** ) judy_strt( _judyarray, ( const unsigned char * ) &key, _depth * JUDY_key_size );
            return mostRecentPair();
        }

        /// retrieve the cell pointer, or return NULL for a given key.
        cvector * find( JudyKey key ) {
            _lastSlot = ( vector ** ) judy_slot( _judyarray, ( const unsigned char * ) &key, _depth * JUDY_key_size );
            if( ( _lastSlot ) && ( * _lastSlot ) ) {
                _success = true;
                return * _lastSlot;
            } else {
                _success = false;
                return 0;
            }
        }

        /// retrieve the key-value pair for the most recent judy query.
        inline const pair mostRecentPair() {
            pair kv;
            judy_key( _judyarray, (unsigned char *) _buff, _depth * JUDY_key_size );
            if( _lastSlot ) {
                kv.value = *_lastSlot;
                _success = true;
            } else {
                kv.value = ( JudyValue ) 0;
                _success = false;
            }
            kv.key = _buff[0];
            return kv;
        }

        /// retrieve the first key-value pair in the array
        const cpair & begin() {
            JudyKey key = 0;
            _lastSlot = ( vector * ) judy_strt( _judyarray, ( const unsigned char * ) &key, _depth * JUDY_key_size );
            return mostRecentPair();
        }

        /// retrieve the last key-value pair in the array
        const cpair & end() {
            _lastSlot = ( vector * ) judy_end( _judyarray );
            return mostRecentPair();
        }

        /// retrieve the key-value pair for the next string in the array.
        const cpair & next() {
            _lastSlot = ( vector * ) judy_nxt( _judyarray );
            return mostRecentPair();
        }

        /// retrieve the key-value pair for the prev string in the array.
        const cpair & previous() {
            _lastSlot = ( vector * ) judy_prv( _judyarray );
            return mostRecentPair();
        }

        /** delete a key-value pair. If the array is not empty,
         * getLastValue() will return the entry before the one that was deleted
         * \sa isEmpty()
         */
        bool removeEntry( JudyKey key ) {
            if( judy_slot( _judyarray, ( const unsigned char * ) &key, sizeof( key ) ) ) {
                _lastSlot = ( vector * ) judy_del( _judyarray );
//                 _lastSlot->~vector(); //for use with placement new
                delete _lastSlot;
                return true;
            } else {
                return false;
            }
        }

        /// true if the array is empty
        bool isEmpty() {
            JudyKey key = 0;
            vector s = ( vector * ) judy_strt( _judyarray, ( const unsigned char * ) &key, _depth * JUDY_key_size );
            return ( s ? false : true );
        }
};
#endif //JUDYL2ARRAY_H