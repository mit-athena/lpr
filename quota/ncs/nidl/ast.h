/*
 * ========================================================================== 
 * Confidential and Proprietary.  Copyright 1987 by Apollo Computer Inc.,
 * Chelmsford, Massachusetts.  Unpublished -- All Rights Reserved Under
 * Copyright Laws Of The United States.
 *
 * Apollo Computer Inc. reserves all rights, title and interest with respect 
 * to copying, modification or the distribution of such software programs and
 * associated documentation, except those rights specifically granted by Apollo
 * in a Product Software Program License, Source Code License or Commercial
 * License Agreement (APOLLO NETWORK COMPUTING SYSTEM) between Apollo and 
 * Licensee.  Without such license agreements, such software programs may not
 * be used, copied, modified or distributed in source or object code form.
 * Further, the copyright notice must appear on the media, the supporting
 * documentation and packaging as set forth in such agreements.  Such License
 * Agreements do not grant any rights to use Apollo Computer's name or trademarks
 * in advertising or publicity, with respect to the distribution of the software
 * programs without the specific prior written permission of Apollo.  Trademark 
 * agreements may be obtained in a separate Trademark License Agreement.
 * ========================================================================== 
 */

/*
 *  This file contains the declarations for the intermediate
 *  form generated by the front end and passed to the backend.
 */


#ifndef asth_incl 
#define asth_incl

#ifdef DSEE
#include "$(rpc.idl).h"
#else
#include "rpc.h"
#endif

#define socket_$num_families 32

#include "nametbl.h"

struct binding_t;
struct field_t;
struct parameter_t;
struct tag_t;
struct type_t;
struct interface_t;
   
/*
 *  C O N S T A N T _ K I N D
 */

typedef enum {
    nil_k,
    integer_k,
    real_k,
    string_k,
    enum_k,
    named_const_k,
    boolean_const_k 
} constant_kind;

/*
 *  C O N S T A N T _ T
 */

typedef struct constant_t {
    int           source_pos;
    constant_kind kind;
    union {
        struct {
            struct type_t  *base_type;
            long            ordinal_mapping;
        } enum_val;
        struct {
            NAMETABLE_id_t  name;
            struct constant_t* resolution;
        } named_val;
        long int     int_val;
        float        real_val;
        STRTAB_str_t string_val;
        boolean      boolean_value;
    } value;
} constant_t;

/*
 *  E N U M E R A T I O N _ T
 */

typedef struct {
    struct  binding_t*  enum_constants;
    long                cardinality;
    boolean             masks_genned;
} enumeration_t;


/* 
 *  A R R A Y _ I N D E X _ T
 */

typedef struct array_index_t {
    struct type_t        *type;
    struct array_index_t *next;
    struct array_index_t *last;
    boolean              adjust;
} array_index_t;

/*
 *  F I X E D _ A R R A Y _ T
 */

typedef struct {             
    struct type_t        *elements; /* element type */
    struct array_index_t *indices;
} fixed_array_t;

/*
 *  O P E N _ A R R A Y _ T
 */

typedef struct {
    struct type_t*        elements;
    struct constant_t*    lower_bound; /* first (open) index type's lower bound */
    struct array_index_t* indices;
} open_array_t;
 
/*
 *  I M P O R T _ T
 */

typedef struct import_t {
    int               source_pos;
    NAMETABLE_id_t    name_of_interface;
    STRTAB_str_t      file_name;
    struct import_t   *next_import;
    struct import_t   *last_import;
} import_t;
   
   
/*
 *  N A M E D _ T
 */

typedef struct {
    NAMETABLE_id_t     name;
    struct type_t*     resolution;
} named_t;

   
/*
 *  P O I N T E R _ T
 */

typedef struct {
    struct type_t *pointee;
    boolean       visited;
    boolean       open;
} pointer_t;

/*
 *  S I D E _ T
 */

typedef enum {
    client_side,
    server_side,
    both_sides
} side_t;

/*
 * L O C A L _ V A R _ T
 */

typedef struct local_var_t {
           NAMETABLE_id_t name;
           NAMETABLE_id_t comment;
           boolean        volatility;
    struct type_t*        type;
           side_t         side;
    struct local_var_t*   next_local;
} local_var_t;


/*
 * H E L P E R S _ T 
 */

typedef struct helpers_t {

    struct helpers_t *next;

    NAMETABLE_id_t cs_mexp;
    NAMETABLE_id_t ss_mexp;

    union {

        struct {
            NAMETABLE_id_t pointer;         
            struct type_t  *pointer_type;
        } scalar_h;

        struct {
            struct helpers_t *fields;
            struct helpers_t *variant;

            NAMETABLE_id_t   alloc_var;
            struct type_t    *alloc_cast_type;
            struct type_t    *alloc_size_type;

            NAMETABLE_id_t   alloc_delta_exp;
            struct type_t    *alloc_delta_type;
        } record_h;

        struct {
            struct helpers_t *element;

            NAMETABLE_id_t   cs_first_element_exp;
            NAMETABLE_id_t   ss_first_element_exp;
            NAMETABLE_id_t   element_ptr_exp;
            NAMETABLE_id_t   index;
            NAMETABLE_id_t   cs_limit_exp;
            NAMETABLE_id_t   ss_limit_exp;
            NAMETABLE_id_t   count_var;

            boolean          check_bound;
            NAMETABLE_id_t   bound_exp;

            boolean          scalar_elements;
            boolean          use_ins_packet;
            NAMETABLE_id_t   ss_arg_exp;

            NAMETABLE_id_t   cs_alloc_exp;
            NAMETABLE_id_t   ss_alloc_exp;

            struct type_t   *alloc_type;

            /* for casting the argument in the call to the manager routine from the
               the server side stub */
            struct type_t   *declared_element_type;

            boolean          fixed_size;
        } array_h;

        struct {
            NAMETABLE_id_t lenvar;
            NAMETABLE_id_t alloc_exp;
        } string_h;

        struct {
            struct helpers_t *tag;
            struct helpers_t *components;
        } variant_h;

        struct {
            struct helpers_t   *fields;
        } component_h;

        struct {
            NAMETABLE_id_t   marshall_ptr_var;
            struct helpers_t *marshall_helpers;
            struct helpers_t *unmarshall_helpers;
        } user_marshalled_h;

    } helpers;
} helpers_t;

/*
 *  P A R A M E T E R _ T
 */

typedef struct parameter_t {    
    int         source_pos;       

    /* parameter content */
    NAMETABLE_id_t      name;
    struct type_t      *type;     
    struct parameter_t *last_param;
    struct parameter_t *next_param;
    struct parameter_t *next_to_marshall;
    struct parameter_t *last_is_ref;
    struct parameter_t *max_is_ref;

    /* backend data */
    helpers_t          *helpers;
    boolean             patched;

    /* parameter attributes */
    boolean             comm_status;
    boolean             in;
    boolean             out;
    boolean             ref;
    boolean             requires_pointer;
    boolean             was_pointer;

} parameter_t;

/*
 *  R O U T I N E
 */

typedef struct {
    int     source_pos;
    /* routine attributes */
    boolean idempotent;
    boolean maybe;
    boolean secure;
    boolean broadcast;
      /* */
    int          op_number;
    parameter_t* parameters;
    parameter_t* result;

    /* backend info */
    parameter_t *first_to_marshall; /* header of list of parameters to un/marshall */
    local_var_t *local_vars;        /* list of helper variables (un)marshalling
                                     the parameters of the routine */
    boolean pointerized;            /* flag: "out parameters have been turned 
                                     into pointers a la C model" */
    boolean backended;              /* flag: "routine node has been munged in
                                     preparation for stub generation" */
    boolean any_ins;                /* flag: routine has one or more [in] parameters */
    boolean any_outs;               /* flag: routine has one or more [out] parameters */
    boolean any_opens;              /* flag: routine has one or more "open" parameters */
    parameter_t *comm_status_param; /* the parameter that comm faults get reported in */
} routine_t;

/*
 *  F I X E D _ S T R I N G _ Z E R O _ T
 */

typedef struct {
    struct array_index_t* index;
} fixed_string_zero_t;

/*
 *  S U B R A N G E _ T
 */

typedef struct {
    constant_t* lower_bound;
    constant_t* upper_bound;
} subrange_t;


/*
 *  F I E L D
 */

typedef struct field_t {
    int            source_pos;
    NAMETABLE_id_t name;
    struct type_t  *type;
    struct field_t *last_is_ref;
    struct field_t *max_is_ref;
    helpers_t      *temp_helpers_p;

    struct field_t *next_field;
    struct field_t *last_field;
} field_t;



/*
 *  S E T _ T
 */

typedef struct {
    struct type_t *base_type;
    boolean       widenable;
} bitset_t;

/*
 *  T A G _ T
 */

typedef struct tag_t {
    int            source_pos;
    constant_t     tag_value;
    struct tag_t*  next_tag;
    struct tag_t*  last_tag;
} tag_t;

/*
 *  C O M P O N E N T _ T
 */

typedef struct c {
    int             source_pos;
    NAMETABLE_id_t  label;
    tag_t          *tags;
    field_t        *fields;
    struct c       *last_component;
    struct c       *next_component;
} component_t;

                                       
/*
 *  V A R I A N T _ T ( rep for tagged unions )
 */

typedef struct {
         int            source_pos;
         NAMETABLE_id_t label;
         NAMETABLE_id_t tag_id;
    struct type_t        *tag_type;
         component_t   *components;
} variant_t;

/*
 *  R E C O R D _ T
 */

typedef struct {
    int             source_pos;
    field_t        *fields;
    variant_t      *variant;
} record_t;

    
/*
 *  U S E R _ M A R S H A L L E D _ T
 */

typedef struct {
    struct type_t        *xmit_type;
    struct type_t        *user_type;
} user_marshalled_t;

    
/*
 *  T Y P E _ K I N D
 */ 

typedef enum {
    /* scalars */
    boolean_k,
    byte_k,
    character_k,

    small_integer_k,      short_integer_k,     long_integer_k,     hyper_integer_k,
    small_unsigned_k,     short_unsigned_k,    long_unsigned_k,    hyper_unsigned_k,
    small_bitset_k,       short_bitset_k,      long_bitset_k,
    small_enumeration_k,  short_enumeration_k, long_enumeration_k,
                          short_real_k,        long_real_k,
    small_subrange_k,     short_subrange_k,    long_subrange_k,
    drep_k,
    handle_k,
    univ_ptr_k,

    /* aggregates */
    open_string_zero_k,
    fixed_string_zero_k,
    open_array_k,
    fixed_array_k,
    open_record_k,  /* created and used by the backend only */
    record_k,
    named_k,
    pointer_k,
    void_k,
    routine_ptr_k,
    reference_k,
    user_marshalled_k
} type_kind;
     
/*
 *  T Y P E _ T
 */

typedef struct type_t {
    int       source_pos;
    /* type attributes */
    NAMETABLE_id_t xmit_type_name;
    struct type_t  *xmit_type;
    NAMETABLE_id_t last_is;
    NAMETABLE_id_t max_is;

    NAMETABLE_id_t type_name;

    type_kind kind;
    union { 
        enumeration_t         enumeration;
        subrange_t            subrange;
        fixed_array_t         fixed_array;
        open_array_t          open_array;
        record_t              record;
        bitset_t              bitset;
        named_t               named;
        pointer_t             pointer;
        routine_t             routine_ptr;
        fixed_string_zero_t   fixed_string_zero;
        pointer_t             reference;
        user_marshalled_t     user_marshalled;
    } type_structure;

    boolean        is_handle;

} type_t;
   
/*
 *  I N T E R F A C E _ T
 */

typedef struct {
    STRTAB_str_t   source_file;
    int            source_pos;

    struct import_t  *imports;     /* for frontend use only */
    struct binding_t *exports;
    int              op_count;     /* for backend use only */
    NAMETABLE_id_t   if_spec_name; /* for backend use only */

    /* interface   attributes */
    uuid_$t        interface_uuid;
    int            interface_version;
    NAMETABLE_id_t implicit_handle_var;  
    type_t         *implicit_handle_type; 
    int            well_known_ports[socket_$num_families];
    boolean        auto_binding;
    boolean        local_only;
} interface_t; 

/*
 *  S T R U C T U R E _ T
 */

typedef union {
    constant_t  constant;
    interface_t interface;
    routine_t   routine;
    type_t      type;
} structure_t;
   

/*
 *  S T R U C T U R E _ K I N D
 */

typedef enum {
    constant_k,
    interface_k,  
    routine_k,
    type_k
} structure_kind;


/*
 *  B I N D I N G _ T
 */

typedef struct binding_t {
    int              source_pos;
    NAMETABLE_id_t   name;
    structure_kind   kind;
    structure_t      *binding;
    struct binding_t *next;       
    struct binding_t *last;
} binding_t;


#endif
