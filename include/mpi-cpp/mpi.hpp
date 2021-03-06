/** **************************************************************************
 * Copyright (C) 2016 Adrien Devresse
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 ** ***************************************************************************/
#ifndef MPI_CPP_MPI_HPP
#define MPI_CPP_MPI_HPP



#include <vector>
#include <bitset>
#include <string>
#include <boost/noncopyable.hpp>

// excepted if disable explicitely
// remove openmpi CXX bindings requirement
#ifndef MPI_CPP_LEGACY_DISABLE
#   ifndef OMPI_SKIP_MPICXX
#       define OMPI_SKIP_MPICXX
#   endif
#endif


#include <mpi.h>

#include "impl/mpi_cpp_config.hpp"
#include "mpi_exception.hpp"


namespace mpi {

template<typename Value> class mpi_future_internal;

class mpi_comm;
class mpi_scope_env;


/**
 * @brief mpi environment scoper
 *
 *  initialize MPI when constructed, finalize MPI when destroyed
 */
class mpi_scope_env : private boost::noncopyable{
public:

    inline mpi_scope_env(int* argc, char*** argv);

    inline virtual ~mpi_scope_env();


    inline void enable_exception_report();

private:
    int initialized;
};



///
/// \brief mpi_future represent the state of an asynchronous MPI request
///
///  follows the C++11 std::future pattern and conventions
///
template<typename Value>
class mpi_future{
public:

    typedef mpi_future<Value> mpi_future_type;
    typedef typename std::vector<mpi_future<Value> > mpi_future_vec;
    typedef typename mpi_future_vec::iterator mpi_future_vec_it;


    ///
    ///  construct a basic mpi_future
    ///  a mpi_future object is valid() only if returned
    ///  by an mpi asynchronous operation
    ///
    /// \brief mpi_future
    ///
    inline mpi_future();

    inline mpi_future(const mpi_future<Value> & f);
    inline mpi_future<Value> & operator=(const mpi_future<Value> & other);

    inline virtual ~mpi_future();

    inline Value get();
    ///
    /// \brief return after the completion of the asynchronous operation
    ///  throw a mpi_invalid_future if called on an invalid or empty mpi_future
    ///
    ///
    inline void wait();


    ///
    /// \brief return after the completion of the asynchronous operation for
    ///  a defined time in micro seconds. If the defined time is 0, execute a non blocking test.
    ///
    ///  throw a mpi_invalid_future if called on an invalid or empty mpi_future
    ///
    /// \return true if completion, false if timeout.
    ///
    inline bool wait_for(std::size_t us_time);


    ///
    /// Check the future validity. Only a future returned by an MPI async operation is valid.
    /// If a valid mpi_future is copied, only the new copy is valid. The original becomes invalid.
    ///
    /// \return true if valid, false otherwise
    ///
    inline bool valid() const;


    ///
    /// Check if the operation already completed, non-blocking
    ///
    /// \return true if completed, false otherwise
    ///
    inline bool completed() const;

    ///
    /// \brief swap two future
    /// \param future to swap
    ///
    void swap(mpi_future<Value> & other);


    ///
    ///  wait for a number 1 to N of mpi_future object to complete
    ///  Every object return is valid and ready for a get() operation
    ///
    ///  Every future returned become invalid in the original buffer
    ///
    /// \param mpi_futures
    /// \return array of iterator to completed futures
    ///
    static mpi_future_vec wait_some(mpi_future_vec & mpi_futures);


    ///
    ///  wait for 1  mpi_future object to complete
    ///  The object return is valid and ready for a get() operation
    ///
    ///  the future object returned becomes invalid in the original buffer
    ///
    /// \param mpi_futures
    /// \return mpi_future object completed, ready for get
    ///
    static mpi_future<Value> wait_any(mpi_future_vec & mpi_futures);

    ///
    ///  wait for a number 1 to N of mpi_future object to complete
    ///  Every object return is valid and ready for a get() operation
    ///
    ///  Every future returned become invalid in the original buffer
    ///
    ///  Wait for a maximum of us_time microseconds, if us_time is 0. This call act as a non blocking MPI_Test
    ///
    /// \param mpi_futures
    /// \return array of iterator to completed futures
    ///
    static mpi_future_vec  wait_some_for(mpi_future_vec & mpi_futures,
                                                         std::size_t us_time);

    ///
    ///  wait for 1  mpi_future object to complete
    ///  The object return is valid and ready for a get() operation
    ///
    ///  the future object returned becomes invalid in the original buffer
    ///
    ///  Wait for a maximum of us_time microseconds, if us_time is 0. This call act as a non blocking MPI_Test
    ///
    /// \param mpi_futures
    /// \return mpi_future object completed, ready for get
    ///
   static mpi_future<Value>  wait_any_for(
            mpi_future_vec & mpi_futures, std::size_t us_time);


    ///
    /// utility function
    ///
    /// remove all invalid futures from mpi_futures_vec
    ///
    /// \param mpi_futures
    /// \return all valid mpi_futures contained in mpi_futures_vec
    ///
    static void filter_invalid(std::vector<mpi_future<Value> > & mpi_futures_vec);


    ///
    /// utility function
    ///
    /// remove all invalid futures from mpi_futures_vec
    ///
    /// \param mpi_futures
    /// \return all valid mpi_futures contained in mpi_futures_vec
    ///
    static void filter_completed(std::vector<mpi_future<Value> > & mpi_futures_vec);


private:


    util::shared_ptr< mpi_future_internal<Value> > _intern;

    inline mpi_future(Value & v, MPI_Request & req);
    friend class mpi_comm;

    template< typename Future>
    friend void manage_completed_multiple_handle(int outcount, std::vector<int> array_indices,
                                                 const std::vector<Future> mpi_futures,
                                                 std::vector<Future> & completed_future);
    template< typename Future>
    friend void manage_completed_single_handle(int index,
                                                    const std::vector<Future> mpi_futures,
                                                    Future & completed_future);

    MPI_Request & get_request();

    Value & get_value();
};



/**
 * @brief mpi communicator
 *
 * bridge to all the MPI functions
 */
class mpi_comm : private boost::noncopyable
{
public:

    class message_handle{
    public:
        inline message_handle();

        ///
        /// \brief get_tag
        /// \return tag associated to the message
        ///
        inline int tag() const;

        ///
        /// \brief get_rank
        /// \return rank of the sending node associated with the message
        ///
        inline int rank() const;

        ///
        /// \brief get_size
        /// \return number of elements of type T part of the message.
        ///  In case of T being a container ( vector, string ), get_size() return the number of container elements
        ///  part of the message
        ///
        template<typename T>
        inline std::size_t count() const;


        ///
        /// \brief test message_handle validity
        /// \return return true if message handle is valid, false if the probe operation has been aborted
        ///
        inline bool is_valid() const;

    private:
        MPI_Status _status;
        MPI_Message _msg;
        friend class mpi_comm;
    };


    /**
     * @brief mpi_comm
     *
     * construct an MPI communication channel
     *
     * MPI operation
     */

    inline mpi_comm();
    inline virtual ~mpi_comm();

    /**
     * @brief rank
     * @return mpi rank of the process
     */
    inline int rank() const{
        return _rank;
    }

    /**
     * @brief size
     * @return size of the mpi communication domain
     */
    inline int size() const{
        return _size;
    }


    /**
     * @brief isMaster
     * @return true if current node is root ( 0 )
     */
    inline bool is_master() const{
        return _rank ==0;
    }

    /// @brief synchronization barrier
    ///
    /// blocking until all the nodes reach it
    inline void barrier();


    /// send local_value to node dest
    /// @param local_value value to send
    /// @param dest node id of the reciver
    /// @param tag identity tag
    template <typename T>
    inline void send(const T & local_value, int dest_node, int tag);

    template <typename T>
    inline void send(const T * value, std::size_t n_value ,
                     int dest_node, int tag);


    /// send local_value to node dest asynchronously
    /// @param local_value value to send
    /// @param dest node id of the reciver
    /// @param tag identity tag
    template <typename T>
    inline mpi_future<T> send_async(const T & local_value, int dest_node, int tag);

    template <typename T>
    inline mpi_future<T*> send_async(const T * value, std::size_t n_value ,
                     int dest_node, int tag);



    /// check for incoming messages, without actual receipt of them
    ///
    /// @param src_node node of the sender
    /// @param tag identity tag of the message
    /// @return messange_handle associated with the incoming message.
    ///         It muse be used with recv(message_handle) to recv the associated
    ///         message
    inline message_handle probe(int src_node, int tag);

    ///
    /// check for incoming messages, without actual receipt of them
    /// if no message are received after a specified time, the function
    /// return an invalid message_handle that need to be tested with
    /// message_handle::is_valid()
    ///
    /// if us_time is 0, a single non-blocking probe is executed
    ///
    /// \param src_node node of the sender
    /// \param tag tag of the message
    /// \param us_time time in micro seconds
    /// \return message_handle, validity test is necessary
    ///
    inline message_handle probe(int src_node, int tag, std::size_t us_time);


    /// received data from an other node
    /// @param src node id of the sender
    /// @param tag identity tag
    /// @return value received
    template <typename T>
    inline void recv(int src_node, int tag, T & value);

    template <typename T>
    inline void recv(const message_handle & handle, T & value);

    template <typename T>
    inline void recv(int src_node, int tag, T* value, std::size_t n_value);


    /// received data from an other node asynchronously
    /// @param src node id of the sender
    /// @param tag identity tag
    /// @return value received
    template <typename T>
    inline mpi_future<T> recv_async(int src_node, int tag);

    template <typename T>
    inline mpi_future<T> recv_async(const message_handle & handle);

    template <typename T>
    inline mpi_future<T*> recv_async(int src_node, int tag, T* value, std::size_t n_value);


    /// return gathered information from all nodes
    /// to all nodes
    ///
    /// @return vector of all the gathered values
    ///
    /// collective operation
    template <typename T, typename Y>
    inline void all_gather(const T & input, std::vector<Y> & output);

    template <typename T, typename Y>
    inline void all_gather(const T* ivalues, std::size_t nvalues, Y* ovalues);

    template <typename T, typename Y>
    inline void all_gather(const T* ivalues, std::size_t nvalues,
                                     Y* ovalues, const std::size_t * ovalues_per_node);


    /// @brief MPI broadcast
    ///
    ///  @param value to broadcast
    ///  @param root node
    ///
    ///  Broadcast a value from node root to all the nodes of
    ///  the communication handle
    ///
    ///
    template <typename T>
    inline void broadcast(T & values, int root);

    template <typename T>
    inline void broadcast(T* buffer, std::size_t nvalues, int root);



    /// @brief return the highest element of the nodes
    /// @param local_value value to compare
    /// @return highest element between all nodes
    ///
    /// collective operation
    template <typename T>
    inline T all_max(const T & elems);

    template <typename T>
    inline void all_max(const T* ivalue, std::size_t n_value, T* ovalue);


    /// @brief return the sum of all the elements of the nodes
    /// @param local_value value to sum
    /// @return sum of all the elements
    ///
    /// collective operation
    template <typename T>
    inline T all_sum(const T & local_value);

    template <typename T>
    inline void all_sum(const T* ivalue, std::size_t n_value, T* ovalue);

private:
    int _rank;
    int _size;
    MPI_Comm _comm;
};



/// various const definitions
const int any_tag = MPI_ANY_TAG;
const int any_source = MPI_ANY_SOURCE;

}


#include "impl/mpi.tcc"

#endif
