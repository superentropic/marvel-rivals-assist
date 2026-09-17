#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>
#include <array>

namespace xor_impl {

	constexpr uint64_t seed_from_file_line( const char* file , int line ) {
		uint64_t h = 0xcbf29ce484222325ULL;
		while ( *file ) {
			h ^= static_cast< uint64_t >( *file++ );
			h *= 0x100000001b3ULL;
		}
		h ^= static_cast< uint64_t >( line ) * 0x9e3779b97f4a7c15ULL;
		return h;
	}

	constexpr uint64_t rotl64( uint64_t x , int k ) {
		return ( x << k ) | ( x >> ( 64 - k ) );
	}

	constexpr uint64_t next_key( uint64_t prev ) {
		prev ^= prev >> 33;
		prev *= 0xff51afd7ed558ccdULL;
		prev ^= prev >> 33;
		prev *= 0xc4ceb9fe1a85ec53ULL;
		prev ^= prev >> 33;
		return prev;
	}

	template<typename CharT , size_t N , uint64_t Key>
	class encrypted_string {
	public:
		constexpr encrypted_string( const CharT( &str )[ N ] ) {
			uint64_t k = Key;
			for ( size_t i = 0; i < N; ++i ) {
				k = next_key( k ^ i );
				uint8_t xor_byte = static_cast< uint8_t >( k & 0xFF );
				data_[ i ] = static_cast< CharT >( static_cast< uint8_t >( str[ i ] ) ^ xor_byte );
			}
		}

		const CharT* decrypt( ) const {
			if ( decrypted_ ) return buf_;
			uint64_t k = Key;
			for ( size_t i = 0; i < N; ++i ) {
				k = next_key( k ^ i );
				uint8_t xor_byte = static_cast< uint8_t >( k & 0xFF );
				buf_[ i ] = static_cast< CharT >( static_cast< uint8_t >( data_[ i ] ) ^ xor_byte );
			}
			decrypted_ = true;
			return buf_;
		}

		void clear( ) const {
			volatile CharT* p = buf_;
			for ( size_t i = 0; i < N; ++i )
				p[ i ] = 0;
			decrypted_ = false;
		}

		size_t size( ) const { return N - 1; }

	private:
		CharT data_[ N ] {};
		mutable CharT buf_[ N ] {};
		mutable bool decrypted_ = false;
	};

	template<typename CharT , size_t N , uint64_t Key>
	class wide_encrypted_string {
	public:
		constexpr wide_encrypted_string( const CharT( &str )[ N ] ) {
			uint64_t k = Key;
			for ( size_t i = 0; i < N; ++i ) {
				k = next_key( k ^ i );
				uint16_t xor_val = static_cast< uint16_t >( k & 0xFFFF );
				data_[ i ] = static_cast< CharT >( static_cast< uint16_t >( str[ i ] ) ^ xor_val );
			}
		}

		const CharT* decrypt( ) const {
			if ( decrypted_ ) return buf_;
			uint64_t k = Key;
			for ( size_t i = 0; i < N; ++i ) {
				k = next_key( k ^ i );
				uint16_t xor_val = static_cast< uint16_t >( k & 0xFFFF );
				buf_[ i ] = static_cast< CharT >( static_cast< uint16_t >( data_[ i ] ) ^ xor_val );
			}
			decrypted_ = true;
			return buf_;
		}

		void clear( ) const {
			volatile CharT* p = buf_;
			for ( size_t i = 0; i < N; ++i )
				p[ i ] = 0;
			decrypted_ = false;
		}

		size_t size( ) const { return N - 1; }

	private:
		CharT data_[ N ] {};
		mutable CharT buf_[ N ] {};
		mutable bool decrypted_ = false;
	};
}

#define XS_KEY __COUNTER__ * 0x9e3779b97f4a7c15ULL + 0xdeadbeefcafebabeULL

#define xs( str )  ( []() -> const char* { \
	static constexpr auto enc = xor_impl::encrypted_string< char , sizeof( str ) , ( __COUNTER__ * 0x9e3779b97f4a7c15ULL + __LINE__ * 0x517cc1b727220a95ULL + 0xdeadbeefcafebabeULL ) >( str ); \
	return enc.decrypt(); \
}() )

#define xsw( str ) ( []() -> const wchar_t* { \
	static constexpr auto enc = xor_impl::wide_encrypted_string< wchar_t , sizeof( str ) / sizeof( wchar_t ) , ( __COUNTER__ * 0x9e3779b97f4a7c15ULL + __LINE__ * 0x517cc1b727220a95ULL + 0xdeadbeefcafebabeULL ) >( str ); \
	return enc.decrypt(); \
}() )
