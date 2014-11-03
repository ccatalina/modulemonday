<?php

class ProfileMemCacheDrupal extends MemCacheDrupal {
  function set($cid, $data, $expire = CACHE_PERMANENT, array $headers = NULL) {
    $created = time();

    // Create new cache object.
    $cache = new stdClass;
    $cache->cid = $cid;
    $cache->data = is_object($data) ? clone $data : $data;
    $cache->created = $created;
    $cache->headers = $headers;
    // Record the previous number of wildcard flushes affecting our cid.
    $cache->flushes = $this->wildcard_flushes($cid);
    if ($expire == CACHE_TEMPORARY) {
      // Convert CACHE_TEMPORARY (-1) into something that will live in memcache
      // until the next flush.
      $cache->expire = REQUEST_TIME + 2591999;
    }
    // Expire time is in seconds if less than 30 days, otherwise is a timestamp.
    else if ($expire != CACHE_PERMANENT && $expire < 2592000) {
      // Expire is expressed in seconds, convert to the proper future timestamp
      // as expected in dmemcache_get().
      $cache->expire = REQUEST_TIME + $expire;
    }
    else {
      $cache->expire = $expire;
    }

    if (variable_get('profile_memcache_cache_set', TRUE)) {
      $cache_as_string = serialize($cache);
      $len_uncompressed = strlen($cache_as_string);
      if ($len_uncompressed > variable_get('profile_memcache_threshold_uncompressed', 1000000)) {
        $len_compressed = strlen(gzcompress($cache_as_string, -1));
        if ($len_compressed > variable_get('profile_memcache_threshold_compressed', 1000000)) {
          $log = array(
            '@bin' => $this->bin,
            '@key' => $cid,
            '@len' => $len_uncompressed,
            '@comp' => $len_compressed,
          );
          watchdog('profile_memcache', 'bin: @bin | key: @key | len: @len | comp: @comp', $log, WATCHDOG_DEBUG);
        }
      }
    }

    // We manually track the expire time in $cache->expire.  When the object
    // expires, we only allow one request to rebuild it to avoid cache
    // stampedes. Other requests for the expired object while it is still being
    // rebuilt get the expired object.
    dmemcache_set($cid, $cache, 0, $this->bin, $this->memcache);
  }
}

