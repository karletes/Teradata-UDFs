/* 
 * FUNCION: NIFVALIDO
 * 
 * Esta UDF en Teradata se encarga de validar y
 * normalizar los codigos NIF espanoles (DNI/NIE/CIF)
 * 
 * PARAMETROS:
 * - Entrada => VARCHAR(20)
 * - Salida  => VARCHAR(20) -- contiene NIF normalizado = CHAR(9)
 * 
 * OPERACIONES:
 *  - elimina caracteres no alfanumericos y convierte letras a mayusc.
 *  - incluye ceros no significativos a la izquierda
 *  - incluye el digito de control cuando recibe un DNI/NIE sin letra
 *  - valida el digito de control recibido en los demas casos
 *  - devuelve una cadena vacia cuando el NIF no pasa la validacion
 * 
 * MIT License (c) 2019
 * This software is supplied as is, with no warranty whatsoever
 * https://github.com/karletes/tera-udf/blob/master/nifvalido.c
 */

typedef enum { false, true } bool;

//-------------------------------------
// valores en formato char & ascii
// 
int ascii( const char *c ){ return (int) *c ; }

bool is_ascii( const char *c, const char *a )
{
  return ascii(c) == ascii(a) ;
}

bool in_ascii( const char *c, const char *a , const char *b )
{
  int x = ascii(c) ;
  return  ascii(a) <= x && x <= ascii(b) ;
}

bool is_char( const char *c )
{
  return in_ascii( c, "A", "Z" ) || in_ascii( c, "a", "z" );
}

bool is_num( const char *c )
{
  return in_ascii( c, "0", "9" );
}

bool is_alnu( const char *c )
{
  return is_num(c) || is_char(c);
}

bool is_nil( const char *c ){ return 0 == ascii(c); }

char to_char( const char *c ){ return (char)ascii(c); }

char to_upper( const char *c )
{
  int n = ascii(c) ;
  return (char)( 97 <= n && n <= 122 ? n -32 : n );
}

//-------------------------------------
// longitud de la cadena
// 
int len( const char *s )
{
  int i = 0 ;
  while( !is_nil( &s[i] )) i += 1 ;
  return i ;
}

//-------------------------------------
// posicion del primer numero significativo
// 
int first( const char *s )
{
  int i ;
  for( i = 0 ; !is_nil( &s[i] ) ; i++ )
  {
    if( i == 0 && is_char( &s[i] ))
    {
      continue ;
    }
    else if( !is_ascii( &s[i], "0" ))
    {
      return i ;
    }
  }
  return -1 ;
}

//-------------------------------------
// posicion del char dentro de la cadena
// 
int find( const char *c, const char *s )
{
  int i ;
  for( i = 0 ; !is_nil( &s[i] ) ; i++ )
  {
    if( is_ascii( &s[i], c )) return i ;
  }
  return -1 ;
}

//-------------------------------------
// copiar solo numeros y letras (mayusc)
// 
void strip( char *des, const char *src )
{
  int  i , j ;
  for( i = j = 0; !is_nil( &src[i] ); i++, j++ )
  {
    if( is_alnu( &src[i] ))
      des[j] = to_upper( &src[i] );
    else j--;
  }
  des[j] = (char)0 ;
}

//-------------------------------------
// copiar cadena a nueva variable
// 
void copy( char *des, const char *src, const int m )
{
  int i ;
  for( i = 0 ; ( m == 0 || m > i ) && !is_nil( &src[i] ); i++ ) des[i] = src[i];
  des[i] = (char)0 ;
}

//-------------------------------------
// convertir cadena a numero
// 
int to_num( char *c )
{
  int n = 0 ;
  for( ; !is_nil( c ); c++ )
  {
    if( !is_num( c )) continue;
    n *= 10 ;
    n +=( ascii(c) - 48 );
  }
  return n ;
}

//-------------------------------------
// normalizar cadena de 9 chars,
// solo alfanumericos y en mayusculas
// 
void normaliza( char *des, const char *src, const int m )
{
  char s[20];
  strip( s, src );
  
  int le = len( s );
  int a  = first( s ), b = 0 ;
  int si = le - a +( is_char( &s[0] ) ? 1 : 0 );
  int to = m  -( is_num( &s[ le-1 ] ) && find( &s[0], "0123456789ZYXWSQP" ) >= 0 ? 1 : 0 );
  int zr = to - si ;
  
  if( a >= 0 && si <= to && find( &s[0], "IKLMOT" ) < 0 )
  {
    if( is_char( &s[0] )) des[b++] = s[0];
    while( zr > 0 ){ des[b++] = to_char("0"); zr--; }
    while( a < le )
    {
      if( a < le -1 && is_char( &s[a] ))
      {
        des[0] = (char)0 ;
        break ;
      }
      des[b] = s[a];
      a++; b++;
    }
    if( to < m ){ des[b] = to_char( "#" ); b++; }
  }
  des[b] = (char)0 ;
}

//-------------------------------------
// calcular letra de NIF normalizado
// 
char letra( char *doc )
{
  if( is_nil( &doc[0] )) return to_char("%");
  
  int n = 0, p = 0, i, a;
  char cntl;
  char nif[] = "TRWAGMYFPDXBNJZSQVHLCKE";
  char cif[] = "JABCDEFGHI";
  
  // CIF
  
  if( in_ascii( &doc[0] ,"A" ,"W" ))
  {
    for( i = 0; i < 8 && !is_nil( &doc[i] ); i++ )
    {
      if( !is_num( &doc[i] )) continue ;
      if( i%2 == 0 )
      {
        p+= ( ascii( &doc[i] ) - 48 );
      }
      else
      {
        a = ( ascii( &doc[i] ) - 48 ) * 2 ;
        n+= ( a % 10 ) + ( a / 10 );
      }
    }
    cntl = doc[i-1];
    n = ( n + p ) % 10 ;
    if( n > 0 ) n = 10 - n ;
    
    if( find( &doc[0], "ABEH" ) >= 0 )  // solo numeros
    {
      return (char)( n + 48 );
    }
    else if( find( &doc[0], "PQSW" ) >= 0 )  // solo letras
    {
      return cif[n];
    }
    return is_num( &cntl ) ? (char)( n + 48 ) : cif[n] ;
  }
  
  // DNI o NIE
  
  n = to_num( &doc[0] );
  
  switch( ascii( &doc[0] ))
  {
    case 89: n += 10000000 ; break;
    case 90: n += 20000000 ; break;
  }
  return n <= 23 ? nif[0] : nif[n%23];
}

//-------------------------------------
// funcion principal,
// punto de entrada desde Teradata
// 
void nifvalido( char *doc, char *result, char sqlstate[6] )
{
  char nif[10];
  normaliza( nif, doc, 9 );
  char cntl = letra( nif );
  
  if( !is_ascii( &nif[8], &cntl ))
  {
    if( is_ascii( &nif[8] ,"#" ) && !is_ascii( &cntl ,"%" ))
      nif[8] = cntl ;
    else 
      nif[0] = (char)0 ;
  }
  copy( result, is_nil( &nif[0] ) ? "         " : nif , 9 );
}
