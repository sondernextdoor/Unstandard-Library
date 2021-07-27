#include <iostream>
#include <Windows.h>



static class ustl
{
private:


	static constexpr size_t npos{ static_cast<size_t>( -1 ) };


	template <class T> 
	class _memory_manager
	{
		void* memory_block{};
		T* client_memory{};
		const size_t type_size{ sizeof( T ) };
		size_t total_size{};
		bool shadow_alloc{ true };

	public:

		_memory_manager( T*& memory )
		{
			client_memory = memory = reinterpret_cast<T*>( 
				memory_block = VirtualAlloc( memory_block, 
											 sizeof( T ), 
											 MEM_COMMIT | MEM_RESERVE, 
											 PAGE_READWRITE ) 
				);
		}


		void* grow( size_t size = sizeof( T ) )
		{
			if ( size == 0 ) { return memory_block; } 
			if ( size == sizeof( T ) && total_size == 0 ) 
			{ 
				total_size += sizeof( T ); 
				return memory_block; 
			}

			VirtualAlloc( static_cast<byte*>( memory_block ) + total_size, 
						  size, 
						  MEM_COMMIT | MEM_RESERVE, 
						  PAGE_READWRITE );

			total_size += size;
			return memory_block;
		}


		void* shrink( size_t size = sizeof( T ) )
		{
			if ( size == 0 || size > total_size || memory_block == nullptr ) { return memory_block; }

			if ( size == total_size ) 
			{ 
				free(); 
				return nullptr; 
			}

			memset( static_cast<byte*>( memory_block ) + ( total_size -= size ), 0, size );
			return memory_block;
		}


		bool copy( void* destination, void* source, size_t size )
		{
			if (   static_cast<byte*>( destination ) == nullptr
				|| static_cast<byte*>( destination ) < static_cast<byte*>( memory_block )
				|| static_cast<byte*>( destination ) >= static_cast<byte*>( memory_block ) + total_size
				|| source == nullptr )
			{
				return false;
			}

			for ( size_t i = 0; i < size; i++ )
			{
				*( static_cast<byte*>( destination ) + i ) = *( static_cast<byte*>( source ) + i );
			}
		}


		void free() 
		{ 
			if ( memory_block != nullptr )
			{
				if ( VirtualFree( memory_block, 0, MEM_RELEASE ) ) 
				{
					memory_block = nullptr;
					total_size = 0;

					if ( shadow_alloc == true ) 
					{
						client_memory = reinterpret_cast<T*>
						( 
							memory_block = VirtualAlloc( memory_block,
														 sizeof(T),
														 MEM_COMMIT | MEM_RESERVE,
														 PAGE_READWRITE ) 
						);
					}
				}
			}; 
		}


		size_t size() const 
		{ 
			return total_size; 
		}


		~_memory_manager() 
		{ 
			shadow_alloc = false;
			free(); 
		};
	};


public:


	template <class T>
	class vector
	{
		_memory_manager<T>* memory_manager{ nullptr };
		T* raw_memory{ 0 };
		size_t element_count{ 0 };
		size_t type_size{ 0 };
		size_t total_size{ 0 };


		void initialize()
		{
			type_size = sizeof( T );

			if ( memory_manager == nullptr )
			{
				memory_manager = new _memory_manager<T>( raw_memory );
			}
		}


		T& grow( size_t count = 1 )
		{
			if ( type_size == 0 || total_size == 0 )
			{
				initialize();
			}

			memory_manager->grow( type_size * count );

			element_count += count;
			total_size = type_size * element_count;
	
			return raw_memory[ ( total_size - type_size ) / type_size ];
		}


	public:


		static constexpr auto npos{ ustl::npos };


		void push_back( T value ) 
		{ 
			grow() = value; 
		}


		template<typename... Args> void push_back_all( T value ) { push_back( value ); }
		template<typename... Args> void push_back_all( T value, Args... args )
		{
			push_back( value );
			push_back_all( args... );
		}


		void pop_back()
		{
			memory_manager->shrink();
			--element_count;
			total_size -= type_size;
		}


		T& at( unsigned int index )
		{
			if ( index >= 0u && index < element_count )
			{
				return raw_memory[ index ];
			}
		}


		void insert( size_t index, T value )
		{
			size_t it{ element_count };
			grow();

			for ( ; it > index; --it ) 
			{
				at( it ) = at( it - 1 );
			}

			at( it ) = value;
		}


		void assign( size_t index, T value )
		{
			at( index ) = value;
		}


		void erase( size_t index ) 
		{ 
			erase( index, index + 1 ); 
		}


		void erase( size_t start_index, size_t end_index )
		{
			if ( start_index > element_count - 1 || end_index > element_count ) { return; }
			if ( start_index == 0 && end_index == element_count - 1 ) { clear(); return; }
			if ( start_index == element_count - 1 ) { pop_back(); return; }

			for ( size_t i = start_index, j = end_index; j < element_count; i++, j++ )
			{
				at( i ) = at( j );
			}

			memory_manager->shrink( type_size * ( end_index - start_index ) );
			element_count -= end_index - start_index;
		}


		void resize( size_t size )
		{
			if ( size == element_count ) { return; }

			if ( size > element_count ) 
			{ 
				grow( size - element_count ); 
				return; 
			}
			
			erase( size, element_count );
		}


		void swap( vector<T>& _vector )
		{
			vector<T> buffer;

			for ( size_t i = 0; i < size(); i++ ) { buffer.push_back( at( i ) ); }
			clear();

			for ( size_t i = 0; i < _vector.size(); i++ ) { push_back( _vector.at( i ) ); }
			_vector.clear();

			for ( size_t i = 0; i < buffer.size(); i++ ) { _vector.push_back( buffer.at( i ) ); }
			buffer.clear();
		}


		size_t find( const T& value )
		{
			for ( size_t i = 0; i < size(); i++ )
			{
				if ( at(i) == value )
				{
					return i;
				}
			}

			return npos;
		}


		bool empty() const 
		{ 
			return ( element_count == 0 && total_size == 0 ); 
		}


		size_t size() const 
		{ 
			return element_count; 
		}


		size_t begin() const 
		{ 
			return 0;  
		}


		size_t end() const 
		{ 
			return element_count - 1;
		}

		
		T& front() 
		{ 
			return at( 0 ); 
		}


		T& back() 
		{ 
			return at( end() ); 
		}


		T* data()
		{
			return raw_memory;
		}


		void clear()
		{
			memory_manager->free();
			element_count = 0;
			total_size = 0;
		}


		void print_elements()
		{
			for ( size_t i = 0; i < size(); i++ )
			{
				std::cout << at( i ) << std::endl;
			}
		}


		T& operator[]( size_t index )
		{
			return raw_memory[ index ];
		}


		vector() = default;
		template <typename... Args> vector( T value, Args... args ) 
		{ 
			if ( type_size == 0 ) { initialize(); }
			push_back_all( value, args... );
		}

		~vector() { clear(); }
	};


	class string
	{
	private:

		vector<char> buffer;

	public:

		static constexpr auto npos{ ustl::npos };


		template <class... Args>
		string(Args... args) { buffer.push_back_all(args...); }

		string( const char* _string )
		{
			for ( size_t i = 0; _string + i != nullptr; i++ )
			{
				buffer.push_back( *( _string + i ) );
				if ( *( _string + i ) == '\0' ) { break; }
			}
		}


		char& at( size_t index )
		{
			return buffer.at( index );
		}


		string& push_back( const char value )
		{
			buffer.push_back( value );
			return *this;
		}


		string& pop_back()
		{
			if ( end() == 0 ) { clear(); return *this; }
			return erase( end() );
		}


		string& insert( size_t index, size_t count, const char value ) 
		{ 
			for ( size_t i = 0; i < count; i++, index++ )
			{
				buffer.insert( index, value );
			}

			return *this;
		}


		string& insert( size_t index, const char* _string )
		{
			for ( size_t i = 0; _string + i != nullptr && *( _string + i ) != '\0'; i++, index++ )
			{
				buffer.insert( index, *( _string + i ) );
			}

			return *this;
		}


		string& append( const char* _string )
		{
			return insert( end(), _string );
		}


		string& replace( size_t start_index, size_t end_index, const char* _string, bool fill = false )
		{
			for ( signed int i = 0; start_index < end_index && _string + i != nullptr && *( _string + i ) != '\0'; i++, start_index++ )
			{
				assign( start_index, *( _string + i ) );
				if  ( fill == true && ( _string + i + 1 == nullptr || *( _string + i + 1 ) == '\0' ) ) { i = -1; }
			}

			return *this;
		}


		string& replace_if( const char _if, const char _with )
		{
			for ( size_t i = 0; i < size(); i++ )
			{
				if ( at( i ) == _if ) { at( i ) = _with; }
			}

			return *this;
		}


		size_t find( const char* _string )
		{
			size_t length{ string_length( _string ) };

			if ( length > size() ) { return npos; }

			for ( size_t i = 0; i < size(); i++ )
			{
				if ( at( i ) == *( _string ) )
				{
					for ( size_t j = 1; j < length; j++ )
					{
						if (at( i + j ) != *( _string + j ) ) { break; }
						if ( j + 1 == length ) { return i; }
					}
				}

				if ( length > size() - i ) { return npos; }
			}
		}


		size_t string_length( const char* _string )
		{
			size_t size{ 0 };

			for ( ; _string + size != nullptr && *( _string + size ) != '\0'; size++ ) {}
			return size;
		}


		size_t string_length( string& _string )
		{
			return string_length( _string.c_str() );
		}


		bool is_equal( string& _string, bool case_insensitive = true )
		{
			return ( ( case_insensitive ? !_stricmp( c_str(), _string.c_str()) : !strcmp( c_str(), _string.c_str() ) ) );
		}


		bool is_equal( const char* _string, bool case_insensitive = true )
		{
			return ( ( case_insensitive ? !_stricmp(c_str(), _string ) : !strcmp(c_str(), _string ) ) );
		}


		char& lower( size_t index ) 
		{ 
			return at( index ) = tolower( at( index ) ); 
		}


		string& lower( size_t start_index, size_t end_index )
		{
			for ( ; start_index < end_index; start_index++ )
			{
				at( start_index ) = tolower( at( start_index ) );
			}

			return *this;
		}


		string& lower()
		{
			for ( size_t i = 0; i < size(); i++)
			{
				at( i ) = tolower( at( i ) );
			}

			return *this;
		}


		char& upper( size_t index ) 
		{ 
			return at( index ) = toupper( at( index ) ); 
		}


		string& upper( size_t start_index, size_t end_index )
		{
			for ( ; start_index < end_index; start_index++ )
			{
				at( start_index ) = toupper( at( start_index ) );
			}

			return *this;
		}


		string& upper()
		{
			for ( size_t i = 0; i < size(); i++ )
			{
				at( i ) = toupper( at( i ) );
			}

			return *this;
		}


		string& assign( size_t index, const char value )
		{
			buffer.assign( index, value );
			return *this;
		}


		string& erase( size_t index )
		{
			buffer.erase( index, index + 1 );
			return *this;
		}


		string& erase( size_t start_index, size_t end_index )
		{
			buffer.erase( start_index, end_index );
			return *this;
		}


		string& resize( size_t size )
		{
			buffer.resize( size );
			return *this;
		}


		bool empty() const
		{
			return buffer.empty();
		}


		size_t size() const
		{
			return buffer.size() - 1;
		}


		size_t begin() const
		{
			return 0;
		}


		size_t end() const
		{
			return buffer.end() - 1;
		}


		char& front()
		{
			return at( 0 );
		}


		char& back()
		{
			return at( end() );
		}


		const char* c_str()
		{
			return &buffer[0];
		}


		void swap( string& _string )
		{
			buffer.swap( _string.buffer );
		}


		void clear()
		{
			buffer.clear();
			
		}


		char& operator[]( size_t index )
		{
			return buffer[ index ];
		}


		bool operator==( string& _string )
		{
			return is_equal( _string, false );
		}


		bool operator!=( string& _string )
		{
			return ( !is_equal( _string, false ) );
		}


		string& operator+( string& _string )
		{
			static string ret_string( c_str() );
			return ret_string.append( _string.c_str() );
		}


		string& operator+( const char* _string )
		{
			static string ret_string( c_str() );
			return ret_string.append( _string );
		}


		string& operator+=( const char* _string )
		{
			return append( _string );
		}


		string& operator+=( string& _string )
		{
			return append( _string.c_str() );
		}


		string& operator=( string _string )
		{
			clear();
			return append( _string.c_str() );
		}


		string& operator=( const char* _string )
		{
			clear();
			return append( _string );
		}


		void print( const char* format = "%s\n" )
		{
			printf( format, c_str() );
		}


		~string() 
		{ 
			clear();  
		}
	};
};


int main()
{
	ustl::vector<int> _vector( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 );
	ustl::string _string( "Test string" );
}