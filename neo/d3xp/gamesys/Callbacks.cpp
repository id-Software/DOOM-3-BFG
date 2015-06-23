// generated file - see CREATE_EVENT_CODE

	/*******************************************************

		1 args

	*******************************************************/

	case 512 :
		typedef void ( idClass::*eventCallback_i_t )( const intptr_t );
		( this->*( eventCallback_i_t )callback )( data[ 0 ] );
		break;

	case 513 :
		typedef void ( idClass::*eventCallback_f_t )( const float );
		( this->*( eventCallback_f_t )callback )( *( float * )&data[ 0 ] );
		break;

	/*******************************************************

		2 args

	*******************************************************/

	case 1024 :
		typedef void ( idClass::*eventCallback_ii_t )( const intptr_t, const intptr_t );
		( this->*( eventCallback_ii_t )callback )( data[ 0 ], data[ 1 ] );
		break;

	case 1025 :
		typedef void ( idClass::*eventCallback_fi_t )( const float, const intptr_t );
		( this->*( eventCallback_fi_t )callback )( *( float * )&data[ 0 ], data[ 1 ] );
		break;

	case 1026 :
		typedef void ( idClass::*eventCallback_if_t )( const intptr_t, const float );
		( this->*( eventCallback_if_t )callback )( data[ 0 ], *( float * )&data[ 1 ] );
		break;

	case 1027 :
		typedef void ( idClass::*eventCallback_ff_t )( const float, const float );
		( this->*( eventCallback_ff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ] );
		break;

	/*******************************************************

		3 args

	*******************************************************/

	case 2048 :
		typedef void ( idClass::*eventCallback_iii_t )( const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ] );
		break;

	case 2049 :
		typedef void ( idClass::*eventCallback_fii_t )( const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ] );
		break;

	case 2050 :
		typedef void ( idClass::*eventCallback_ifi_t )( const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ] );
		break;

	case 2051 :
		typedef void ( idClass::*eventCallback_ffi_t )( const float, const float, const intptr_t );
		( this->*( eventCallback_ffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ] );
		break;

	case 2052 :
		typedef void ( idClass::*eventCallback_iif_t )( const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ] );
		break;

	case 2053 :
		typedef void ( idClass::*eventCallback_fif_t )( const float, const intptr_t, const float );
		( this->*( eventCallback_fif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ] );
		break;

	case 2054 :
		typedef void ( idClass::*eventCallback_iff_t )( const intptr_t, const float, const float );
		( this->*( eventCallback_iff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ] );
		break;

	case 2055 :
		typedef void ( idClass::*eventCallback_fff_t )( const float, const float, const float );
		( this->*( eventCallback_fff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ] );
		break;

	/*******************************************************

		4 args

	*******************************************************/

	case 4096 :
		typedef void ( idClass::*eventCallback_iiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ] );
		break;

	case 4097 :
		typedef void ( idClass::*eventCallback_fiii_t )( const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ] );
		break;

	case 4098 :
		typedef void ( idClass::*eventCallback_ifii_t )( const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ] );
		break;

	case 4099 :
		typedef void ( idClass::*eventCallback_ffii_t )( const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ] );
		break;

	case 4100 :
		typedef void ( idClass::*eventCallback_iifi_t )( const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ] );
		break;

	case 4101 :
		typedef void ( idClass::*eventCallback_fifi_t )( const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ] );
		break;

	case 4102 :
		typedef void ( idClass::*eventCallback_iffi_t )( const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ] );
		break;

	case 4103 :
		typedef void ( idClass::*eventCallback_fffi_t )( const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ] );
		break;

	case 4104 :
		typedef void ( idClass::*eventCallback_iiif_t )( const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4105 :
		typedef void ( idClass::*eventCallback_fiif_t )( const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4106 :
		typedef void ( idClass::*eventCallback_ifif_t )( const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4107 :
		typedef void ( idClass::*eventCallback_ffif_t )( const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4108 :
		typedef void ( idClass::*eventCallback_iiff_t )( const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4109 :
		typedef void ( idClass::*eventCallback_fiff_t )( const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4110 :
		typedef void ( idClass::*eventCallback_ifff_t )( const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ] );
		break;

	case 4111 :
		typedef void ( idClass::*eventCallback_ffff_t )( const float, const float, const float, const float );
		( this->*( eventCallback_ffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ] );
		break;

	/*******************************************************

		5 args

	*******************************************************/

	case 8192 :
		typedef void ( idClass::*eventCallback_iiiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8193 :
		typedef void ( idClass::*eventCallback_fiiii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8194 :
		typedef void ( idClass::*eventCallback_ifiii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8195 :
		typedef void ( idClass::*eventCallback_ffiii_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8196 :
		typedef void ( idClass::*eventCallback_iifii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8197 :
		typedef void ( idClass::*eventCallback_fifii_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8198 :
		typedef void ( idClass::*eventCallback_iffii_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8199 :
		typedef void ( idClass::*eventCallback_fffii_t )( const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ] );
		break;

	case 8200 :
		typedef void ( idClass::*eventCallback_iiifi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8201 :
		typedef void ( idClass::*eventCallback_fiifi_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8202 :
		typedef void ( idClass::*eventCallback_ififi_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ififi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8203 :
		typedef void ( idClass::*eventCallback_ffifi_t )( const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8204 :
		typedef void ( idClass::*eventCallback_iiffi_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8205 :
		typedef void ( idClass::*eventCallback_fiffi_t )( const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8206 :
		typedef void ( idClass::*eventCallback_ifffi_t )( const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ifffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8207 :
		typedef void ( idClass::*eventCallback_ffffi_t )( const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ffffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ] );
		break;

	case 8208 :
		typedef void ( idClass::*eventCallback_iiiif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8209 :
		typedef void ( idClass::*eventCallback_fiiif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8210 :
		typedef void ( idClass::*eventCallback_ifiif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8211 :
		typedef void ( idClass::*eventCallback_ffiif_t )( const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8212 :
		typedef void ( idClass::*eventCallback_iifif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iifif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8213 :
		typedef void ( idClass::*eventCallback_fifif_t )( const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8214 :
		typedef void ( idClass::*eventCallback_iffif_t )( const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8215 :
		typedef void ( idClass::*eventCallback_fffif_t )( const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8216 :
		typedef void ( idClass::*eventCallback_iiiff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8217 :
		typedef void ( idClass::*eventCallback_fiiff_t )( const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8218 :
		typedef void ( idClass::*eventCallback_ififf_t )( const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ififf_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8219 :
		typedef void ( idClass::*eventCallback_ffiff_t )( const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8220 :
		typedef void ( idClass::*eventCallback_iifff_t )( const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iifff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8221 :
		typedef void ( idClass::*eventCallback_fifff_t )( const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8222 :
		typedef void ( idClass::*eventCallback_iffff_t )( const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	case 8223 :
		typedef void ( idClass::*eventCallback_fffff_t )( const float, const float, const float, const float, const float );
		( this->*( eventCallback_fffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ] );
		break;

	/*******************************************************

		6 args

	*******************************************************/

	case 16384 :
		typedef void ( idClass::*eventCallback_iiiiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16385 :
		typedef void ( idClass::*eventCallback_fiiiii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16386 :
		typedef void ( idClass::*eventCallback_ifiiii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16387 :
		typedef void ( idClass::*eventCallback_ffiiii_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16388 :
		typedef void ( idClass::*eventCallback_iifiii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16389 :
		typedef void ( idClass::*eventCallback_fifiii_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16390 :
		typedef void ( idClass::*eventCallback_iffiii_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16391 :
		typedef void ( idClass::*eventCallback_fffiii_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16392 :
		typedef void ( idClass::*eventCallback_iiifii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiifii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16393 :
		typedef void ( idClass::*eventCallback_fiifii_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16394 :
		typedef void ( idClass::*eventCallback_ififii_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16395 :
		typedef void ( idClass::*eventCallback_ffifii_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16396 :
		typedef void ( idClass::*eventCallback_iiffii_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiffii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16397 :
		typedef void ( idClass::*eventCallback_fiffii_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16398 :
		typedef void ( idClass::*eventCallback_ifffii_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16399 :
		typedef void ( idClass::*eventCallback_ffffii_t )( const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ] );
		break;

	case 16400 :
		typedef void ( idClass::*eventCallback_iiiifi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiiifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16401 :
		typedef void ( idClass::*eventCallback_fiiifi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16402 :
		typedef void ( idClass::*eventCallback_ifiifi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16403 :
		typedef void ( idClass::*eventCallback_ffiifi_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16404 :
		typedef void ( idClass::*eventCallback_iififi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iififi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16405 :
		typedef void ( idClass::*eventCallback_fififi_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fififi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16406 :
		typedef void ( idClass::*eventCallback_iffifi_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iffifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16407 :
		typedef void ( idClass::*eventCallback_fffifi_t )( const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fffifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16408 :
		typedef void ( idClass::*eventCallback_iiiffi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiiffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16409 :
		typedef void ( idClass::*eventCallback_fiiffi_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16410 :
		typedef void ( idClass::*eventCallback_ififfi_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ififfi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16411 :
		typedef void ( idClass::*eventCallback_ffiffi_t )( const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ffiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16412 :
		typedef void ( idClass::*eventCallback_iifffi_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iifffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16413 :
		typedef void ( idClass::*eventCallback_fifffi_t )( const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fifffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16414 :
		typedef void ( idClass::*eventCallback_iffffi_t )( const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iffffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16415 :
		typedef void ( idClass::*eventCallback_fffffi_t )( const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fffffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ] );
		break;

	case 16416 :
		typedef void ( idClass::*eventCallback_iiiiif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16417 :
		typedef void ( idClass::*eventCallback_fiiiif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16418 :
		typedef void ( idClass::*eventCallback_ifiiif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16419 :
		typedef void ( idClass::*eventCallback_ffiiif_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16420 :
		typedef void ( idClass::*eventCallback_iifiif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iifiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16421 :
		typedef void ( idClass::*eventCallback_fifiif_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fifiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16422 :
		typedef void ( idClass::*eventCallback_iffiif_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iffiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16423 :
		typedef void ( idClass::*eventCallback_fffiif_t )( const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fffiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16424 :
		typedef void ( idClass::*eventCallback_iiifif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iiifif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16425 :
		typedef void ( idClass::*eventCallback_fiifif_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fiifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16426 :
		typedef void ( idClass::*eventCallback_ififif_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ififif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16427 :
		typedef void ( idClass::*eventCallback_ffifif_t )( const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ffifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16428 :
		typedef void ( idClass::*eventCallback_iiffif_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iiffif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16429 :
		typedef void ( idClass::*eventCallback_fiffif_t )( const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fiffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16430 :
		typedef void ( idClass::*eventCallback_ifffif_t )( const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ifffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16431 :
		typedef void ( idClass::*eventCallback_ffffif_t )( const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16432 :
		typedef void ( idClass::*eventCallback_iiiiff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16433 :
		typedef void ( idClass::*eventCallback_fiiiff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16434 :
		typedef void ( idClass::*eventCallback_ifiiff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ifiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16435 :
		typedef void ( idClass::*eventCallback_ffiiff_t )( const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16436 :
		typedef void ( idClass::*eventCallback_iififf_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iififf_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16437 :
		typedef void ( idClass::*eventCallback_fififf_t )( const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fififf_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16438 :
		typedef void ( idClass::*eventCallback_iffiff_t )( const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iffiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16439 :
		typedef void ( idClass::*eventCallback_fffiff_t )( const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fffiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16440 :
		typedef void ( idClass::*eventCallback_iiifff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iiifff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16441 :
		typedef void ( idClass::*eventCallback_fiifff_t )( const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fiifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16442 :
		typedef void ( idClass::*eventCallback_ififff_t )( const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ififff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16443 :
		typedef void ( idClass::*eventCallback_ffifff_t )( const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ffifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16444 :
		typedef void ( idClass::*eventCallback_iiffff_t )( const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iiffff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16445 :
		typedef void ( idClass::*eventCallback_fiffff_t )( const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_fiffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16446 :
		typedef void ( idClass::*eventCallback_ifffff_t )( const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ifffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	case 16447 :
		typedef void ( idClass::*eventCallback_ffffff_t )( const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ffffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ] );
		break;

	/*******************************************************

		7 args

	*******************************************************/

	case 32768 :
		typedef void ( idClass::*eventCallback_iiiiiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiiiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32769 :
		typedef void ( idClass::*eventCallback_fiiiiii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32770 :
		typedef void ( idClass::*eventCallback_ifiiiii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32771 :
		typedef void ( idClass::*eventCallback_ffiiiii_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32772 :
		typedef void ( idClass::*eventCallback_iifiiii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifiiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32773 :
		typedef void ( idClass::*eventCallback_fifiiii_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32774 :
		typedef void ( idClass::*eventCallback_iffiiii_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32775 :
		typedef void ( idClass::*eventCallback_fffiiii_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32776 :
		typedef void ( idClass::*eventCallback_iiifiii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiifiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32777 :
		typedef void ( idClass::*eventCallback_fiifiii_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiifiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32778 :
		typedef void ( idClass::*eventCallback_ififiii_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32779 :
		typedef void ( idClass::*eventCallback_ffifiii_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffifiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32780 :
		typedef void ( idClass::*eventCallback_iiffiii_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiffiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32781 :
		typedef void ( idClass::*eventCallback_fiffiii_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiffiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32782 :
		typedef void ( idClass::*eventCallback_ifffiii_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifffiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32783 :
		typedef void ( idClass::*eventCallback_ffffiii_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffffiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32784 :
		typedef void ( idClass::*eventCallback_iiiifii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiifii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32785 :
		typedef void ( idClass::*eventCallback_fiiifii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32786 :
		typedef void ( idClass::*eventCallback_ifiifii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32787 :
		typedef void ( idClass::*eventCallback_ffiifii_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32788 :
		typedef void ( idClass::*eventCallback_iififii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iififii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32789 :
		typedef void ( idClass::*eventCallback_fififii_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fififii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32790 :
		typedef void ( idClass::*eventCallback_iffifii_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32791 :
		typedef void ( idClass::*eventCallback_fffifii_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32792 :
		typedef void ( idClass::*eventCallback_iiiffii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiffii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32793 :
		typedef void ( idClass::*eventCallback_fiiffii_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32794 :
		typedef void ( idClass::*eventCallback_ififfii_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififfii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32795 :
		typedef void ( idClass::*eventCallback_ffiffii_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32796 :
		typedef void ( idClass::*eventCallback_iifffii_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifffii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32797 :
		typedef void ( idClass::*eventCallback_fifffii_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32798 :
		typedef void ( idClass::*eventCallback_iffffii_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32799 :
		typedef void ( idClass::*eventCallback_fffffii_t )( const float, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ] );
		break;

	case 32800 :
		typedef void ( idClass::*eventCallback_iiiiifi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiiiifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32801 :
		typedef void ( idClass::*eventCallback_fiiiifi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiiiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32802 :
		typedef void ( idClass::*eventCallback_ifiiifi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifiiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32803 :
		typedef void ( idClass::*eventCallback_ffiiifi_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffiiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32804 :
		typedef void ( idClass::*eventCallback_iifiifi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iifiifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32805 :
		typedef void ( idClass::*eventCallback_fifiifi_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fifiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32806 :
		typedef void ( idClass::*eventCallback_iffiifi_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iffiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32807 :
		typedef void ( idClass::*eventCallback_fffiifi_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fffiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32808 :
		typedef void ( idClass::*eventCallback_iiififi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiififi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32809 :
		typedef void ( idClass::*eventCallback_fiififi_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiififi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32810 :
		typedef void ( idClass::*eventCallback_ifififi_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifififi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32811 :
		typedef void ( idClass::*eventCallback_ffififi_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffififi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32812 :
		typedef void ( idClass::*eventCallback_iiffifi_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiffifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32813 :
		typedef void ( idClass::*eventCallback_fiffifi_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiffifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32814 :
		typedef void ( idClass::*eventCallback_ifffifi_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifffifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32815 :
		typedef void ( idClass::*eventCallback_ffffifi_t )( const float, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffffifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32816 :
		typedef void ( idClass::*eventCallback_iiiiffi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiiiffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32817 :
		typedef void ( idClass::*eventCallback_fiiiffi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiiiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32818 :
		typedef void ( idClass::*eventCallback_ifiiffi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ifiiffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32819 :
		typedef void ( idClass::*eventCallback_ffiiffi_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ffiiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32820 :
		typedef void ( idClass::*eventCallback_iififfi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iififfi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32821 :
		typedef void ( idClass::*eventCallback_fififfi_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fififfi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32822 :
		typedef void ( idClass::*eventCallback_iffiffi_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iffiffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32823 :
		typedef void ( idClass::*eventCallback_fffiffi_t )( const float, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fffiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32824 :
		typedef void ( idClass::*eventCallback_iiifffi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iiifffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32825 :
		typedef void ( idClass::*eventCallback_fiifffi_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fiifffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32826 :
		typedef void ( idClass::*eventCallback_ififffi_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ififffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32827 :
		typedef void ( idClass::*eventCallback_ffifffi_t )( const float, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ffifffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32828 :
		typedef void ( idClass::*eventCallback_iiffffi_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iiffffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32829 :
		typedef void ( idClass::*eventCallback_fiffffi_t )( const float, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fiffffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32830 :
		typedef void ( idClass::*eventCallback_ifffffi_t )( const intptr_t, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ifffffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32831 :
		typedef void ( idClass::*eventCallback_ffffffi_t )( const float, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ffffffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ] );
		break;

	case 32832 :
		typedef void ( idClass::*eventCallback_iiiiiif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiiiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32833 :
		typedef void ( idClass::*eventCallback_fiiiiif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32834 :
		typedef void ( idClass::*eventCallback_ifiiiif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifiiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32835 :
		typedef void ( idClass::*eventCallback_ffiiiif_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32836 :
		typedef void ( idClass::*eventCallback_iifiiif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iifiiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32837 :
		typedef void ( idClass::*eventCallback_fifiiif_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fifiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32838 :
		typedef void ( idClass::*eventCallback_iffiiif_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iffiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32839 :
		typedef void ( idClass::*eventCallback_fffiiif_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fffiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32840 :
		typedef void ( idClass::*eventCallback_iiifiif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiifiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32841 :
		typedef void ( idClass::*eventCallback_fiifiif_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiifiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32842 :
		typedef void ( idClass::*eventCallback_ififiif_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ififiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32843 :
		typedef void ( idClass::*eventCallback_ffifiif_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffifiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32844 :
		typedef void ( idClass::*eventCallback_iiffiif_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiffiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32845 :
		typedef void ( idClass::*eventCallback_fiffiif_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiffiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32846 :
		typedef void ( idClass::*eventCallback_ifffiif_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifffiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32847 :
		typedef void ( idClass::*eventCallback_ffffiif_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffffiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32848 :
		typedef void ( idClass::*eventCallback_iiiifif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iiiifif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32849 :
		typedef void ( idClass::*eventCallback_fiiifif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fiiifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32850 :
		typedef void ( idClass::*eventCallback_ifiifif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ifiifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32851 :
		typedef void ( idClass::*eventCallback_ffiifif_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ffiifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32852 :
		typedef void ( idClass::*eventCallback_iififif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iififif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32853 :
		typedef void ( idClass::*eventCallback_fififif_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fififif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32854 :
		typedef void ( idClass::*eventCallback_iffifif_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iffifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32855 :
		typedef void ( idClass::*eventCallback_fffifif_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fffifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32856 :
		typedef void ( idClass::*eventCallback_iiiffif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iiiffif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32857 :
		typedef void ( idClass::*eventCallback_fiiffif_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fiiffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32858 :
		typedef void ( idClass::*eventCallback_ififfif_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ififfif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32859 :
		typedef void ( idClass::*eventCallback_ffiffif_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffiffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32860 :
		typedef void ( idClass::*eventCallback_iifffif_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iifffif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32861 :
		typedef void ( idClass::*eventCallback_fifffif_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fifffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32862 :
		typedef void ( idClass::*eventCallback_iffffif_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iffffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32863 :
		typedef void ( idClass::*eventCallback_fffffif_t )( const float, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fffffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32864 :
		typedef void ( idClass::*eventCallback_iiiiiff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiiiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32865 :
		typedef void ( idClass::*eventCallback_fiiiiff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32866 :
		typedef void ( idClass::*eventCallback_ifiiiff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ifiiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32867 :
		typedef void ( idClass::*eventCallback_ffiiiff_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32868 :
		typedef void ( idClass::*eventCallback_iifiiff_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iifiiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32869 :
		typedef void ( idClass::*eventCallback_fifiiff_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fifiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32870 :
		typedef void ( idClass::*eventCallback_iffiiff_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iffiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32871 :
		typedef void ( idClass::*eventCallback_fffiiff_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fffiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32872 :
		typedef void ( idClass::*eventCallback_iiififf_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iiififf_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32873 :
		typedef void ( idClass::*eventCallback_fiififf_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fiififf_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32874 :
		typedef void ( idClass::*eventCallback_ifififf_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ifififf_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32875 :
		typedef void ( idClass::*eventCallback_ffififf_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ffififf_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32876 :
		typedef void ( idClass::*eventCallback_iiffiff_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iiffiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32877 :
		typedef void ( idClass::*eventCallback_fiffiff_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fiffiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32878 :
		typedef void ( idClass::*eventCallback_ifffiff_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ifffiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32879 :
		typedef void ( idClass::*eventCallback_ffffiff_t )( const float, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ffffiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32880 :
		typedef void ( idClass::*eventCallback_iiiifff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iiiifff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32881 :
		typedef void ( idClass::*eventCallback_fiiifff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fiiifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32882 :
		typedef void ( idClass::*eventCallback_ifiifff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ifiifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32883 :
		typedef void ( idClass::*eventCallback_ffiifff_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ffiifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32884 :
		typedef void ( idClass::*eventCallback_iififff_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iififff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32885 :
		typedef void ( idClass::*eventCallback_fififff_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fififff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32886 :
		typedef void ( idClass::*eventCallback_iffifff_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iffifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32887 :
		typedef void ( idClass::*eventCallback_fffifff_t )( const float, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fffifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32888 :
		typedef void ( idClass::*eventCallback_iiiffff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iiiffff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32889 :
		typedef void ( idClass::*eventCallback_fiiffff_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_fiiffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32890 :
		typedef void ( idClass::*eventCallback_ififfff_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_ififfff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32891 :
		typedef void ( idClass::*eventCallback_ffiffff_t )( const float, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_ffiffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32892 :
		typedef void ( idClass::*eventCallback_iifffff_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_iifffff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32893 :
		typedef void ( idClass::*eventCallback_fifffff_t )( const float, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_fifffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32894 :
		typedef void ( idClass::*eventCallback_iffffff_t )( const intptr_t, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_iffffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	case 32895 :
		typedef void ( idClass::*eventCallback_fffffff_t )( const float, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_fffffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ] );
		break;

	/*******************************************************

		8 args

	*******************************************************/

	case 65536 :
		typedef void ( idClass::*eventCallback_iiiiiiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiiiiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65537 :
		typedef void ( idClass::*eventCallback_fiiiiiii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiiiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65538 :
		typedef void ( idClass::*eventCallback_ifiiiiii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiiiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65539 :
		typedef void ( idClass::*eventCallback_ffiiiiii_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiiiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65540 :
		typedef void ( idClass::*eventCallback_iifiiiii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifiiiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65541 :
		typedef void ( idClass::*eventCallback_fifiiiii_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifiiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65542 :
		typedef void ( idClass::*eventCallback_iffiiiii_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffiiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65543 :
		typedef void ( idClass::*eventCallback_fffiiiii_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffiiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65544 :
		typedef void ( idClass::*eventCallback_iiifiiii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiifiiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65545 :
		typedef void ( idClass::*eventCallback_fiifiiii_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiifiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65546 :
		typedef void ( idClass::*eventCallback_ififiiii_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65547 :
		typedef void ( idClass::*eventCallback_ffifiiii_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffifiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65548 :
		typedef void ( idClass::*eventCallback_iiffiiii_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiffiiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65549 :
		typedef void ( idClass::*eventCallback_fiffiiii_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiffiiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65550 :
		typedef void ( idClass::*eventCallback_ifffiiii_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifffiiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65551 :
		typedef void ( idClass::*eventCallback_ffffiiii_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffffiiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65552 :
		typedef void ( idClass::*eventCallback_iiiifiii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiifiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65553 :
		typedef void ( idClass::*eventCallback_fiiifiii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiifiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65554 :
		typedef void ( idClass::*eventCallback_ifiifiii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiifiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65555 :
		typedef void ( idClass::*eventCallback_ffiifiii_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiifiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65556 :
		typedef void ( idClass::*eventCallback_iififiii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iififiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65557 :
		typedef void ( idClass::*eventCallback_fififiii_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fififiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65558 :
		typedef void ( idClass::*eventCallback_iffifiii_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffifiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65559 :
		typedef void ( idClass::*eventCallback_fffifiii_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffifiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65560 :
		typedef void ( idClass::*eventCallback_iiiffiii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiffiii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65561 :
		typedef void ( idClass::*eventCallback_fiiffiii_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiffiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65562 :
		typedef void ( idClass::*eventCallback_ififfiii_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififfiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65563 :
		typedef void ( idClass::*eventCallback_ffiffiii_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiffiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65564 :
		typedef void ( idClass::*eventCallback_iifffiii_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifffiii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65565 :
		typedef void ( idClass::*eventCallback_fifffiii_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifffiii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65566 :
		typedef void ( idClass::*eventCallback_iffffiii_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffffiii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65567 :
		typedef void ( idClass::*eventCallback_fffffiii_t )( const float, const float, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffffiii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65568 :
		typedef void ( idClass::*eventCallback_iiiiifii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiiifii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65569 :
		typedef void ( idClass::*eventCallback_fiiiifii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiiifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65570 :
		typedef void ( idClass::*eventCallback_ifiiifii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiiifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65571 :
		typedef void ( idClass::*eventCallback_ffiiifii_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiiifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65572 :
		typedef void ( idClass::*eventCallback_iifiifii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iifiifii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65573 :
		typedef void ( idClass::*eventCallback_fifiifii_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fifiifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65574 :
		typedef void ( idClass::*eventCallback_iffiifii_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffiifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65575 :
		typedef void ( idClass::*eventCallback_fffiifii_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffiifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65576 :
		typedef void ( idClass::*eventCallback_iiififii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiififii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65577 :
		typedef void ( idClass::*eventCallback_fiififii_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiififii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65578 :
		typedef void ( idClass::*eventCallback_ifififii_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifififii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65579 :
		typedef void ( idClass::*eventCallback_ffififii_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffififii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65580 :
		typedef void ( idClass::*eventCallback_iiffifii_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiffifii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65581 :
		typedef void ( idClass::*eventCallback_fiffifii_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiffifii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65582 :
		typedef void ( idClass::*eventCallback_ifffifii_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifffifii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65583 :
		typedef void ( idClass::*eventCallback_ffffifii_t )( const float, const float, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffffifii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65584 :
		typedef void ( idClass::*eventCallback_iiiiffii_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiiiffii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65585 :
		typedef void ( idClass::*eventCallback_fiiiffii_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiiiffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65586 :
		typedef void ( idClass::*eventCallback_ifiiffii_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifiiffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65587 :
		typedef void ( idClass::*eventCallback_ffiiffii_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffiiffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65588 :
		typedef void ( idClass::*eventCallback_iififfii_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iififfii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65589 :
		typedef void ( idClass::*eventCallback_fififfii_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fififfii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65590 :
		typedef void ( idClass::*eventCallback_iffiffii_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iffiffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65591 :
		typedef void ( idClass::*eventCallback_fffiffii_t )( const float, const float, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fffiffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65592 :
		typedef void ( idClass::*eventCallback_iiifffii_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiifffii_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65593 :
		typedef void ( idClass::*eventCallback_fiifffii_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiifffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65594 :
		typedef void ( idClass::*eventCallback_ififffii_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ififffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65595 :
		typedef void ( idClass::*eventCallback_ffifffii_t )( const float, const float, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffifffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65596 :
		typedef void ( idClass::*eventCallback_iiffffii_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_iiffffii_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65597 :
		typedef void ( idClass::*eventCallback_fiffffii_t )( const float, const intptr_t, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_fiffffii_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65598 :
		typedef void ( idClass::*eventCallback_ifffffii_t )( const intptr_t, const float, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ifffffii_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65599 :
		typedef void ( idClass::*eventCallback_ffffffii_t )( const float, const float, const float, const float, const float, const float, const intptr_t, const intptr_t );
		( this->*( eventCallback_ffffffii_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], data[ 7 ] );
		break;

	case 65600 :
		typedef void ( idClass::*eventCallback_iiiiiifi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiiiiifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65601 :
		typedef void ( idClass::*eventCallback_fiiiiifi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiiiiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65602 :
		typedef void ( idClass::*eventCallback_ifiiiifi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifiiiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65603 :
		typedef void ( idClass::*eventCallback_ffiiiifi_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffiiiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65604 :
		typedef void ( idClass::*eventCallback_iifiiifi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iifiiifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65605 :
		typedef void ( idClass::*eventCallback_fifiiifi_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fifiiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65606 :
		typedef void ( idClass::*eventCallback_iffiiifi_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iffiiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65607 :
		typedef void ( idClass::*eventCallback_fffiiifi_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fffiiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65608 :
		typedef void ( idClass::*eventCallback_iiifiifi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiifiifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65609 :
		typedef void ( idClass::*eventCallback_fiifiifi_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiifiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65610 :
		typedef void ( idClass::*eventCallback_ififiifi_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ififiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65611 :
		typedef void ( idClass::*eventCallback_ffifiifi_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffifiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65612 :
		typedef void ( idClass::*eventCallback_iiffiifi_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiffiifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65613 :
		typedef void ( idClass::*eventCallback_fiffiifi_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiffiifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65614 :
		typedef void ( idClass::*eventCallback_ifffiifi_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifffiifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65615 :
		typedef void ( idClass::*eventCallback_ffffiifi_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffffiifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65616 :
		typedef void ( idClass::*eventCallback_iiiififi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiiififi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65617 :
		typedef void ( idClass::*eventCallback_fiiififi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiiififi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65618 :
		typedef void ( idClass::*eventCallback_ifiififi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ifiififi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65619 :
		typedef void ( idClass::*eventCallback_ffiififi_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffiififi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65620 :
		typedef void ( idClass::*eventCallback_iifififi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iifififi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65621 :
		typedef void ( idClass::*eventCallback_fifififi_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fifififi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65622 :
		typedef void ( idClass::*eventCallback_iffififi_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iffififi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65623 :
		typedef void ( idClass::*eventCallback_fffififi_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fffififi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65624 :
		typedef void ( idClass::*eventCallback_iiiffifi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iiiffifi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65625 :
		typedef void ( idClass::*eventCallback_fiiffifi_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fiiffifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65626 :
		typedef void ( idClass::*eventCallback_ififfifi_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ififfifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65627 :
		typedef void ( idClass::*eventCallback_ffiffifi_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_ffiffifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65628 :
		typedef void ( idClass::*eventCallback_iifffifi_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iifffifi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65629 :
		typedef void ( idClass::*eventCallback_fifffifi_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fifffifi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65630 :
		typedef void ( idClass::*eventCallback_iffffifi_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_iffffifi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65631 :
		typedef void ( idClass::*eventCallback_fffffifi_t )( const float, const float, const float, const float, const float, const intptr_t, const float, const intptr_t );
		( this->*( eventCallback_fffffifi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65632 :
		typedef void ( idClass::*eventCallback_iiiiiffi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiiiiffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65633 :
		typedef void ( idClass::*eventCallback_fiiiiffi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiiiiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65634 :
		typedef void ( idClass::*eventCallback_ifiiiffi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ifiiiffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65635 :
		typedef void ( idClass::*eventCallback_ffiiiffi_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ffiiiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65636 :
		typedef void ( idClass::*eventCallback_iifiiffi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iifiiffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65637 :
		typedef void ( idClass::*eventCallback_fifiiffi_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fifiiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65638 :
		typedef void ( idClass::*eventCallback_iffiiffi_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iffiiffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65639 :
		typedef void ( idClass::*eventCallback_fffiiffi_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fffiiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65640 :
		typedef void ( idClass::*eventCallback_iiififfi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiififfi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65641 :
		typedef void ( idClass::*eventCallback_fiififfi_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiififfi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65642 :
		typedef void ( idClass::*eventCallback_ifififfi_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ifififfi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65643 :
		typedef void ( idClass::*eventCallback_ffififfi_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ffififfi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65644 :
		typedef void ( idClass::*eventCallback_iiffiffi_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_iiffiffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65645 :
		typedef void ( idClass::*eventCallback_fiffiffi_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_fiffiffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65646 :
		typedef void ( idClass::*eventCallback_ifffiffi_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ifffiffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65647 :
		typedef void ( idClass::*eventCallback_ffffiffi_t )( const float, const float, const float, const float, const intptr_t, const float, const float, const intptr_t );
		( this->*( eventCallback_ffffiffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65648 :
		typedef void ( idClass::*eventCallback_iiiifffi_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iiiifffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65649 :
		typedef void ( idClass::*eventCallback_fiiifffi_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fiiifffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65650 :
		typedef void ( idClass::*eventCallback_ifiifffi_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ifiifffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65651 :
		typedef void ( idClass::*eventCallback_ffiifffi_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ffiifffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65652 :
		typedef void ( idClass::*eventCallback_iififffi_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iififffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65653 :
		typedef void ( idClass::*eventCallback_fififffi_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fififffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65654 :
		typedef void ( idClass::*eventCallback_iffifffi_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iffifffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65655 :
		typedef void ( idClass::*eventCallback_fffifffi_t )( const float, const float, const float, const intptr_t, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fffifffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65656 :
		typedef void ( idClass::*eventCallback_iiiffffi_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iiiffffi_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65657 :
		typedef void ( idClass::*eventCallback_fiiffffi_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fiiffffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65658 :
		typedef void ( idClass::*eventCallback_ififfffi_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ififfffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65659 :
		typedef void ( idClass::*eventCallback_ffiffffi_t )( const float, const float, const intptr_t, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_ffiffffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65660 :
		typedef void ( idClass::*eventCallback_iifffffi_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iifffffi_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65661 :
		typedef void ( idClass::*eventCallback_fifffffi_t )( const float, const intptr_t, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fifffffi_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65662 :
		typedef void ( idClass::*eventCallback_iffffffi_t )( const intptr_t, const float, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_iffffffi_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65663 :
		typedef void ( idClass::*eventCallback_fffffffi_t )( const float, const float, const float, const float, const float, const float, const float, const intptr_t );
		( this->*( eventCallback_fffffffi_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], data[ 7 ] );
		break;

	case 65664 :
		typedef void ( idClass::*eventCallback_iiiiiiif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiiiiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65665 :
		typedef void ( idClass::*eventCallback_fiiiiiif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiiiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65666 :
		typedef void ( idClass::*eventCallback_ifiiiiif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifiiiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65667 :
		typedef void ( idClass::*eventCallback_ffiiiiif_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiiiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65668 :
		typedef void ( idClass::*eventCallback_iifiiiif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iifiiiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65669 :
		typedef void ( idClass::*eventCallback_fifiiiif_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fifiiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65670 :
		typedef void ( idClass::*eventCallback_iffiiiif_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iffiiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65671 :
		typedef void ( idClass::*eventCallback_fffiiiif_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fffiiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65672 :
		typedef void ( idClass::*eventCallback_iiifiiif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiifiiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65673 :
		typedef void ( idClass::*eventCallback_fiifiiif_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiifiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65674 :
		typedef void ( idClass::*eventCallback_ififiiif_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ififiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65675 :
		typedef void ( idClass::*eventCallback_ffifiiif_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffifiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65676 :
		typedef void ( idClass::*eventCallback_iiffiiif_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiffiiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65677 :
		typedef void ( idClass::*eventCallback_fiffiiif_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiffiiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65678 :
		typedef void ( idClass::*eventCallback_ifffiiif_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifffiiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65679 :
		typedef void ( idClass::*eventCallback_ffffiiif_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffffiiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65680 :
		typedef void ( idClass::*eventCallback_iiiifiif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiifiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65681 :
		typedef void ( idClass::*eventCallback_fiiifiif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiifiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65682 :
		typedef void ( idClass::*eventCallback_ifiifiif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ifiifiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65683 :
		typedef void ( idClass::*eventCallback_ffiifiif_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiifiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65684 :
		typedef void ( idClass::*eventCallback_iififiif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iififiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65685 :
		typedef void ( idClass::*eventCallback_fififiif_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fififiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65686 :
		typedef void ( idClass::*eventCallback_iffifiif_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iffifiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65687 :
		typedef void ( idClass::*eventCallback_fffifiif_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fffifiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65688 :
		typedef void ( idClass::*eventCallback_iiiffiif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iiiffiif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65689 :
		typedef void ( idClass::*eventCallback_fiiffiif_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fiiffiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65690 :
		typedef void ( idClass::*eventCallback_ififfiif_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ififfiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65691 :
		typedef void ( idClass::*eventCallback_ffiffiif_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_ffiffiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65692 :
		typedef void ( idClass::*eventCallback_iifffiif_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iifffiif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65693 :
		typedef void ( idClass::*eventCallback_fifffiif_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fifffiif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65694 :
		typedef void ( idClass::*eventCallback_iffffiif_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_iffffiif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65695 :
		typedef void ( idClass::*eventCallback_fffffiif_t )( const float, const float, const float, const float, const float, const intptr_t, const intptr_t, const float );
		( this->*( eventCallback_fffffiif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65696 :
		typedef void ( idClass::*eventCallback_iiiiifif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iiiiifif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65697 :
		typedef void ( idClass::*eventCallback_fiiiifif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fiiiifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65698 :
		typedef void ( idClass::*eventCallback_ifiiifif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ifiiifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65699 :
		typedef void ( idClass::*eventCallback_ffiiifif_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ffiiifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65700 :
		typedef void ( idClass::*eventCallback_iifiifif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iifiifif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65701 :
		typedef void ( idClass::*eventCallback_fifiifif_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fifiifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65702 :
		typedef void ( idClass::*eventCallback_iffiifif_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iffiifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65703 :
		typedef void ( idClass::*eventCallback_fffiifif_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fffiifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65704 :
		typedef void ( idClass::*eventCallback_iiififif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iiififif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65705 :
		typedef void ( idClass::*eventCallback_fiififif_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fiififif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65706 :
		typedef void ( idClass::*eventCallback_ifififif_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ifififif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65707 :
		typedef void ( idClass::*eventCallback_ffififif_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ffififif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65708 :
		typedef void ( idClass::*eventCallback_iiffifif_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_iiffifif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65709 :
		typedef void ( idClass::*eventCallback_fiffifif_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_fiffifif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65710 :
		typedef void ( idClass::*eventCallback_ifffifif_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ifffifif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65711 :
		typedef void ( idClass::*eventCallback_ffffifif_t )( const float, const float, const float, const float, const intptr_t, const float, const intptr_t, const float );
		( this->*( eventCallback_ffffifif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65712 :
		typedef void ( idClass::*eventCallback_iiiiffif_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iiiiffif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65713 :
		typedef void ( idClass::*eventCallback_fiiiffif_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fiiiffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65714 :
		typedef void ( idClass::*eventCallback_ifiiffif_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ifiiffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65715 :
		typedef void ( idClass::*eventCallback_ffiiffif_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffiiffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65716 :
		typedef void ( idClass::*eventCallback_iififfif_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iififfif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65717 :
		typedef void ( idClass::*eventCallback_fififfif_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fififfif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65718 :
		typedef void ( idClass::*eventCallback_iffiffif_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iffiffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65719 :
		typedef void ( idClass::*eventCallback_fffiffif_t )( const float, const float, const float, const intptr_t, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fffiffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65720 :
		typedef void ( idClass::*eventCallback_iiifffif_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iiifffif_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65721 :
		typedef void ( idClass::*eventCallback_fiifffif_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fiifffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65722 :
		typedef void ( idClass::*eventCallback_ififffif_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ififffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65723 :
		typedef void ( idClass::*eventCallback_ffifffif_t )( const float, const float, const intptr_t, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffifffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65724 :
		typedef void ( idClass::*eventCallback_iiffffif_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_iiffffif_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65725 :
		typedef void ( idClass::*eventCallback_fiffffif_t )( const float, const intptr_t, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_fiffffif_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65726 :
		typedef void ( idClass::*eventCallback_ifffffif_t )( const intptr_t, const float, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ifffffif_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65727 :
		typedef void ( idClass::*eventCallback_ffffffif_t )( const float, const float, const float, const float, const float, const float, const intptr_t, const float );
		( this->*( eventCallback_ffffffif_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65728 :
		typedef void ( idClass::*eventCallback_iiiiiiff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiiiiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65729 :
		typedef void ( idClass::*eventCallback_fiiiiiff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiiiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65730 :
		typedef void ( idClass::*eventCallback_ifiiiiff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ifiiiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65731 :
		typedef void ( idClass::*eventCallback_ffiiiiff_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiiiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65732 :
		typedef void ( idClass::*eventCallback_iifiiiff_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iifiiiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65733 :
		typedef void ( idClass::*eventCallback_fifiiiff_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fifiiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65734 :
		typedef void ( idClass::*eventCallback_iffiiiff_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iffiiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65735 :
		typedef void ( idClass::*eventCallback_fffiiiff_t )( const float, const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fffiiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65736 :
		typedef void ( idClass::*eventCallback_iiifiiff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiifiiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65737 :
		typedef void ( idClass::*eventCallback_fiifiiff_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiifiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65738 :
		typedef void ( idClass::*eventCallback_ififiiff_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ififiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65739 :
		typedef void ( idClass::*eventCallback_ffifiiff_t )( const float, const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ffifiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65740 :
		typedef void ( idClass::*eventCallback_iiffiiff_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_iiffiiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65741 :
		typedef void ( idClass::*eventCallback_fiffiiff_t )( const float, const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_fiffiiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65742 :
		typedef void ( idClass::*eventCallback_ifffiiff_t )( const intptr_t, const float, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ifffiiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65743 :
		typedef void ( idClass::*eventCallback_ffffiiff_t )( const float, const float, const float, const float, const intptr_t, const intptr_t, const float, const float );
		( this->*( eventCallback_ffffiiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65744 :
		typedef void ( idClass::*eventCallback_iiiififf_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiififf_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65745 :
		typedef void ( idClass::*eventCallback_fiiififf_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiififf_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65746 :
		typedef void ( idClass::*eventCallback_ifiififf_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ifiififf_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65747 :
		typedef void ( idClass::*eventCallback_ffiififf_t )( const float, const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiififf_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65748 :
		typedef void ( idClass::*eventCallback_iifififf_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iifififf_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65749 :
		typedef void ( idClass::*eventCallback_fifififf_t )( const float, const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fifififf_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65750 :
		typedef void ( idClass::*eventCallback_iffififf_t )( const intptr_t, const float, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iffififf_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65751 :
		typedef void ( idClass::*eventCallback_fffififf_t )( const float, const float, const float, const intptr_t, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fffififf_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65752 :
		typedef void ( idClass::*eventCallback_iiiffiff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iiiffiff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65753 :
		typedef void ( idClass::*eventCallback_fiiffiff_t )( const float, const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fiiffiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65754 :
		typedef void ( idClass::*eventCallback_ififfiff_t )( const intptr_t, const float, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ififfiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65755 :
		typedef void ( idClass::*eventCallback_ffiffiff_t )( const float, const float, const intptr_t, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_ffiffiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65756 :
		typedef void ( idClass::*eventCallback_iifffiff_t )( const intptr_t, const intptr_t, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iifffiff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65757 :
		typedef void ( idClass::*eventCallback_fifffiff_t )( const float, const intptr_t, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fifffiff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65758 :
		typedef void ( idClass::*eventCallback_iffffiff_t )( const intptr_t, const float, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_iffffiff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65759 :
		typedef void ( idClass::*eventCallback_fffffiff_t )( const float, const float, const float, const float, const float, const intptr_t, const float, const float );
		( this->*( eventCallback_fffffiff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65760 :
		typedef void ( idClass::*eventCallback_iiiiifff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iiiiifff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65761 :
		typedef void ( idClass::*eventCallback_fiiiifff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fiiiifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65762 :
		typedef void ( idClass::*eventCallback_ifiiifff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ifiiifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65763 :
		typedef void ( idClass::*eventCallback_ffiiifff_t )( const float, const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ffiiifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65764 :
		typedef void ( idClass::*eventCallback_iifiifff_t )( const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iifiifff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65765 :
		typedef void ( idClass::*eventCallback_fifiifff_t )( const float, const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fifiifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65766 :
		typedef void ( idClass::*eventCallback_iffiifff_t )( const intptr_t, const float, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iffiifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65767 :
		typedef void ( idClass::*eventCallback_fffiifff_t )( const float, const float, const float, const intptr_t, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fffiifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65768 :
		typedef void ( idClass::*eventCallback_iiififff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iiififff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65769 :
		typedef void ( idClass::*eventCallback_fiififff_t )( const float, const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fiififff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65770 :
		typedef void ( idClass::*eventCallback_ifififff_t )( const intptr_t, const float, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ifififff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65771 :
		typedef void ( idClass::*eventCallback_ffififff_t )( const float, const float, const intptr_t, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ffififff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65772 :
		typedef void ( idClass::*eventCallback_iiffifff_t )( const intptr_t, const intptr_t, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_iiffifff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65773 :
		typedef void ( idClass::*eventCallback_fiffifff_t )( const float, const intptr_t, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_fiffifff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65774 :
		typedef void ( idClass::*eventCallback_ifffifff_t )( const intptr_t, const float, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ifffifff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65775 :
		typedef void ( idClass::*eventCallback_ffffifff_t )( const float, const float, const float, const float, const intptr_t, const float, const float, const float );
		( this->*( eventCallback_ffffifff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65776 :
		typedef void ( idClass::*eventCallback_iiiiffff_t )( const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iiiiffff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65777 :
		typedef void ( idClass::*eventCallback_fiiiffff_t )( const float, const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_fiiiffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65778 :
		typedef void ( idClass::*eventCallback_ifiiffff_t )( const intptr_t, const float, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_ifiiffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65779 :
		typedef void ( idClass::*eventCallback_ffiiffff_t )( const float, const float, const intptr_t, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_ffiiffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65780 :
		typedef void ( idClass::*eventCallback_iififfff_t )( const intptr_t, const intptr_t, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iififfff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65781 :
		typedef void ( idClass::*eventCallback_fififfff_t )( const float, const intptr_t, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_fififfff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65782 :
		typedef void ( idClass::*eventCallback_iffiffff_t )( const intptr_t, const float, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_iffiffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65783 :
		typedef void ( idClass::*eventCallback_fffiffff_t )( const float, const float, const float, const intptr_t, const float, const float, const float, const float );
		( this->*( eventCallback_fffiffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65784 :
		typedef void ( idClass::*eventCallback_iiifffff_t )( const intptr_t, const intptr_t, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_iiifffff_t )callback )( data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65785 :
		typedef void ( idClass::*eventCallback_fiifffff_t )( const float, const intptr_t, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_fiifffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65786 :
		typedef void ( idClass::*eventCallback_ififffff_t )( const intptr_t, const float, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ififffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65787 :
		typedef void ( idClass::*eventCallback_ffifffff_t )( const float, const float, const intptr_t, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ffifffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65788 :
		typedef void ( idClass::*eventCallback_iiffffff_t )( const intptr_t, const intptr_t, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_iiffffff_t )callback )( data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65789 :
		typedef void ( idClass::*eventCallback_fiffffff_t )( const float, const intptr_t, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_fiffffff_t )callback )( *( float * )&data[ 0 ], data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65790 :
		typedef void ( idClass::*eventCallback_ifffffff_t )( const intptr_t, const float, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ifffffff_t )callback )( data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

	case 65791 :
		typedef void ( idClass::*eventCallback_ffffffff_t )( const float, const float, const float, const float, const float, const float, const float, const float );
		( this->*( eventCallback_ffffffff_t )callback )( *( float * )&data[ 0 ], *( float * )&data[ 1 ], *( float * )&data[ 2 ], *( float * )&data[ 3 ], *( float * )&data[ 4 ], *( float * )&data[ 5 ], *( float * )&data[ 6 ], *( float * )&data[ 7 ] );
		break;

