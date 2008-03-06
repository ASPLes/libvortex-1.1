/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez N� 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcal� de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */
#include <vortex.h>
#define LOG_DOMAIN "vortex-hash"

struct _VortexHash {
	axlHash        * table;
	VortexMutex      mutex;

	/* configuration functions */
	axlHashFunc    hash_func;
	axlEqualFunc   key_equal_func;

	/* destroy functions */
	axlDestroyFunc   key_destroy;
	axlDestroyFunc   value_destroy;
};

/**
 * \defgroup vortex_hash VortexHash: Thread Safe Hash table used inside Vortex Library.
 */

/**
 * \addtogroup vortex_hash
 * @{
 */

/**
 * @brief Creates a new VortexHash setting all functions.
 * 
 * Creates a new Vortex Hash Table. All vortex library is programed
 * making heavy use of hash tables so things can go pretty much
 * faster.
 *
 * But this makes race condition to appear anywhere so, this type
 * allow vortex library to create critical section to all operation
 * that are applied to a hash table.
 *
 * @param hash_func
 * @param key_equal_func
 * @param key_destroy_func
 * @param value_destroy_func
 *
 * @return a new VortexHash table or NULL if fail
 **/
VortexHash * vortex_hash_new_full (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func,
				   axlDestroyFunc key_destroy_func,
				   axlDestroyFunc value_destroy_func)
{
	VortexHash * result;

	result                 = axl_new (VortexHash, 1);
	result->table          = axl_hash_new (hash_func, key_equal_func);

	/* configuration functions */
	result->hash_func      = hash_func;
	result->key_equal_func = key_equal_func;

	/* destroy functions */
	result->key_destroy    = key_destroy_func;
	result->value_destroy  = value_destroy_func;

	/* the mutex */
	vortex_mutex_create (&result->mutex);
	
	return result;
	
}

/**
 * @brief Creates a new VortexHash without providing destroy function.
 * 
 * A vortex_hash_new_full version passing in as NULL key_destroy_func and 
 * value_destroy_func. 
 *
 * @param hash_func
 * @param key_equal_func
 * 
 * @return a newly created VortexHash or NULL if fails.
 **/
VortexHash * vortex_hash_new      (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func)
{
	return vortex_hash_new_full (hash_func, key_equal_func, NULL, NULL);
}

/**
 * @brief Inserts a pair key/value inside the given VortexHash
 *
 * 
 * Insert a key/value pair into hash_table.
 *
 * @param hash_table: the hash table
 * @param key: the key to insert
 * @param value: the value to insert
 **/
void         vortex_hash_insert   (VortexHash *hash_table,
				   axlPointer key,
				   axlPointer value)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	vortex_mutex_lock   (&hash_table->mutex);

	axl_hash_insert_full (hash_table->table, 
			      key, hash_table->key_destroy, 
			      value, hash_table->value_destroy);

	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/**
 * @brief Replace using the given pair key/value into the given hash.
 * 
 * Replace the key/value pair into hash_table. If previous value key/value
 * is not found then the pair is simply added.
 *
 * @param hash_table the hash table to operate on
 * @param key the key value
 * @param value the value to insert
 **/
void         vortex_hash_replace  (VortexHash *hash_table,
				   axlPointer key,
				   axlPointer value)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	vortex_mutex_lock   (&hash_table->mutex);
	
	axl_hash_insert_full (hash_table->table, 
			      key, hash_table->key_destroy, 
			      value, hash_table->value_destroy);

	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/** 
 * @brief Replace using the given pair key/value into the given hash,
 * providing the particular key and value destroy function, overrding
 * default ones.
 * 
 * Replace the key/value pair into hash_table. If previous value key/value
 * is not found then the pair is simply added.
 *
 * @param hash_table the hash table to operate on
 * @param key the key value
 * @param key_destroy Destroy function to be called for the key.
 * @param value the value to insert
 * @param value_destroy Destroy value function to be called for the data.
 */
void         vortex_hash_replace_full  (VortexHash     * hash_table,
					axlPointer       key,
					axlDestroyFunc   key_destroy,
					axlPointer       value,
					axlDestroyFunc   value_destroy)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return;
	vortex_mutex_lock   (&hash_table->mutex);
	
	axl_hash_insert_full (hash_table->table, 
			      key, key_destroy,
			      value, value_destroy);

	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/**
 * @brief Returns the current hash size.
 *
 * 
 * Returns the hash table size
 * 
 * @param hash_table the hash table to operate on.
 *
 * @return the hash table size or -1 if fails
 **/
int      vortex_hash_size     (VortexHash *hash_table)
{
	int result;

	/* check hash table reference */
	if (hash_table == NULL)
		return -1;
	vortex_mutex_lock     (&hash_table->mutex);

	result = axl_hash_items (hash_table->table);

	vortex_mutex_unlock   (&hash_table->mutex);	
	
	return result;
}

/**
 * @brief Perform a lookup using the given key inside the given hash.
 *
 * 
 * Return the value, if found, associated with the key.
 * 
 * @param hash_table the hash table
 * @param key the key value
 *
 * @return the value found or NULL if fails
 **/
axlPointer   vortex_hash_lookup   (VortexHash *hash_table,
				   axlPointer  key)
{
	axlPointer data;

	/* check hash table reference */
	if (hash_table == NULL)
		return NULL;
	vortex_mutex_lock   (&hash_table->mutex);
	
	data = axl_hash_get (hash_table->table, key);

	vortex_mutex_unlock (&hash_table->mutex);	

	return data;
}

/** 
 * @brief Allows to get the data pointed by the provided key and
 * removing it from the table in one step.
 * 
 * Return the value, if found, associated with the key.
 * 
 * @param hash_table the hash table
 * @param key the key value
 *
 * @return the value found or NULL if fails
 */
axlPointer   vortex_hash_lookup_and_clear   (VortexHash   *hash_table,
					     axlPointer    key)
{
	
	axlPointer data;

	/* check hash table reference */
	if (hash_table == NULL)
		return NULL;
	vortex_mutex_lock   (&hash_table->mutex);
	
	/* get the data */
	data = axl_hash_get (hash_table->table, key);

	/* remove the data */
	axl_hash_remove (hash_table->table, key);

	/* unlock and return */
	vortex_mutex_unlock (&hash_table->mutex);	

	return data;
}

/**
 * @brief Removes the value index by the given key inside the given hash.
 *
 * 
 * Remove a key/pair value from the hash
 * 
 * @param hash_table the hash table
 * @param key the key value to lookup and destroy
 *
 * @return true if found and removed and false if not removed
 **/
bool         vortex_hash_remove   (VortexHash *hash_table,
				   axlPointer key)
{
	/* check hash table reference */
	if (hash_table == NULL)
		return false;

	vortex_mutex_lock   (&hash_table->mutex);

	axl_hash_remove (hash_table->table, key);
	
	vortex_mutex_unlock (&hash_table->mutex);

	return true;
}

/**
 * @brief Destroy the given hash freeing all resources.
 * 
 * Destroy the hash table.
 *
 * @param hash_table the hash table to operate on.
 **/
void         vortex_hash_destroy  (VortexHash *hash_table)
{

	/* check hash table reference */
	if (hash_table == NULL)
		return;

	/* look the mutex */
	vortex_mutex_lock (&hash_table->mutex);
	
	/* get a reference to the table and nullify it */
	axl_hash_free (hash_table->table);
	hash_table->table = NULL;

	/* unlock and free */
	vortex_mutex_unlock (&hash_table->mutex);
	vortex_mutex_destroy (&hash_table->mutex);

	/* release the node holding all information */
	axl_free       (hash_table);

	return;
}

/** 
 * @brief Allows to remove the provided key and its associated data on
 * the provided hash without calling to the optionally associated
 * destroy functions.
 * 
 * @param hash_table The hash where the unlink operation will take
 * place.
 *
 * @param key The key for the data to be removed.
 * 
 * @return true if the item was removed (current implementation always
 * return true).
 */
bool         vortex_hash_delete   (VortexHash   *hash_table,
				   axlPointer    key)
{
	v_return_val_if_fail (hash_table, false);

	vortex_mutex_lock    (&hash_table->mutex);

	axl_hash_delete      (hash_table->table, key);
	
	vortex_mutex_unlock  (&hash_table->mutex);

	return true;
}

/**
 * @brief Perform a foreach over all elements inside the VortexHash.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided.
 *
 * @param user_data User defined data to be provided to the handler.
 **/
void         vortex_hash_foreach  (VortexHash         *hash_table,
				   axlHashForeachFunc  func,
				   axlPointer         user_data)
{
	/* check references */
	v_return_if_fail (hash_table);
	v_return_if_fail (func);
	v_return_if_fail (hash_table->table);

	vortex_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach    (hash_table->table, func, user_data);
	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/**
 * @brief Perform a foreach over all elements inside the VortexHash,
 * allowing to provide two user defined reference at the handler.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided (two references).
 *
 * @param user_data User defined data to be provided to the handler.
 *
 * @param user_data2 Second user defined data to be provided to the
 * handler.
 **/
void         vortex_hash_foreach2  (VortexHash           *hash_table,
				    axlHashForeachFunc2   func,
				    axlPointer            user_data,
				    axlPointer            user_data2)
{
	if (hash_table == NULL || func == NULL || hash_table->table == NULL)
		return;

	vortex_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach2   (hash_table->table, func, user_data, user_data2);
	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/**
 * @brief Perform a foreach over all elements inside the VortexHash,
 * allowing to provide three user defined reference at the handler.
 * 
 * @param hash_table The hash table where the foreach operation will
 * be implemented.
 *
 * @param func User defined handler called for each item found, along
 * with the user defined data provided (two references).
 *
 * @param user_data User defined data to be provided to the handler.
 *
 * @param user_data2 Second user defined data to be provided to the
 * handler.
 *
 * @param user_data3 Third user defined data to be provided to the
 * handler.
 **/
void         vortex_hash_foreach3  (VortexHash         * hash_table,
				    axlHashForeachFunc3  func,
				    axlPointer           user_data,
				    axlPointer           user_data2,
				    axlPointer           user_data3)
{
	if (hash_table == NULL || func == NULL || hash_table->table == NULL)
		return;

	vortex_mutex_lock   (&hash_table->mutex);
	axl_hash_foreach3   (hash_table->table, func, user_data, user_data2, user_data3);
	vortex_mutex_unlock (&hash_table->mutex);
}

/** 
 * @internal
 * @brief Support function for \ref vortex_hash_clear function.
 * 
 * It just returns true.
 */
bool     vortex_hash_clear_allways_true (axlPointer key, axlPointer value, axlPointer user_data) 
{
	return true;
}


/** 
 * @brief Allows to clear a hash table.
 * 
 * @param hash_table The hash table to clear
 */
void         vortex_hash_clear    (VortexHash *hash_table)
{
	if (hash_table == NULL)
		return;

	vortex_mutex_lock (&hash_table->mutex);

	axl_hash_free (hash_table->table);
	hash_table->table = axl_hash_new (hash_table->hash_func, 
					  hash_table->key_equal_func);

	vortex_mutex_unlock (&hash_table->mutex);

	return;
}

/* @} */
