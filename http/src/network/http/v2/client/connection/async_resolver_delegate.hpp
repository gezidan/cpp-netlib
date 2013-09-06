// Copyright (C) 2013 by Glyn Matthews
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef __NETWORK_HTTP_V2_CLIENT_CONNECTION_ASYNC_RESOLVER_DELEGATE_INC__
#define __NETWORK_HTTP_V2_CLIENT_CONNECTION_ASYNC_RESOLVER_DELEGATE_INC__

#include "network/http/v2/client/connection/resolver_delegate.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/exception/all.hpp>
#include <stdexcept>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace network {
  namespace http {
    namespace v2 {
      /*!
       * \class async_resolver_delegate network::http::v2::async_resolver_delegate <network/http/v2/client/connection/async_resolver_delegate.hpp>
       * \brief Resolves and maintains a cache of hosts.
       */
      class async_resolver_delegate : public resolver_delegate {

	async_resolver_delegate(const async_resolver_delegate &) = delete;
	async_resolver_delegate &operator = (const async_resolver_delegate &) = delete;

      public:

	/*!
	 * \brief Constructor.
	 */
	async_resolver_delegate(boost::asio::io_service &service, bool cache_resolved = false)
	  : resolver_(service)
	  , resolver_strand_(new boost::asio::io_service::strand(service))
	  , cache_resolved_(cache_resolved) {

	}

	virtual ~async_resolver_delegate() noexcept {

	}

	/*!
	 * \brief Resolves a host asynchronously.
	 */
	virtual void resolve(const std::string &host, std::uint16_t port, on_resolved_fn on_resolved) {
	  if (cache_resolved_) {
	    endpoint_cache::iterator it = endpoint_cache_.find(boost::to_lower_copy(host));
	    if (it != endpoint_cache_.end()) {
	      boost::system::error_code ignored;
	      on_resolved(ignored, it->second);
	      return;
	    }
	  }

	  resolver::query query(host, std::to_string(port));
	  resolver_.async_resolve(query,
              resolver_strand_->wrap(
	          [=](const boost::system::error_code &ec,
		      resolver_iterator endpoint_iterator) {

		    auto resolvers = std::make_pair(endpoint_iterator, resolver_iterator());
		    if (!ec && cache_resolved_) {
		      endpoint_cache_.insert(std::make_pair(host, resolvers));
		    }
		    on_resolved(ec, resolvers);
		  }));
	}

	/*!
	 * \brief Clears the cache of already resolved endpoints.
	 */
	void clear_resolved_cache() {
	  endpoint_cache().swap(endpoint_cache_);
	}

      private:

	typedef boost::asio::io_service::strand strand;
	typedef std::unordered_map<std::string, resolver_iterator_range> endpoint_cache;

	resolver resolver_;
	std::unique_ptr<strand> resolver_strand_;
	bool cache_resolved_;
	endpoint_cache endpoint_cache_;

      };
    } // namespace v2
  } // namespace http
} // namespace network

#endif // __NETWORK_HTTP_V2_CLIENT_CONNECTION_ASYNC_RESOLVER_DELEGATE_INC__