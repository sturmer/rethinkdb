# Release 1.16.0 (Stand By Me)

Released on 2015-01-29

The highlights of this release are:
* A new administration API
* Changefeeds on complex queries
* Numerous improvements and enhancements throughout RethinkDB

Read the [release blog post][1.16-blog] for more details.

[1.16-blog]: http://rethinkdb.com/blog/1.16-release/

## Compatibility ##

### Backwards-compatible changes ###

Data files from RethinkDB versions 1.13.0 onward will be automatically
migrated to version 1.16.x. As with any major release, back up your data files
before performing the upgrade. If you are upgrading from a release earlier
than 1.13.0, follow the [migration instructions][migration-documentation] before upgrading.

[migration-documentation]: http://rethinkdb.com/docs/migration/

Secondary indexes now use a new format; old indexes will continue to work, but
you should rebuild indexes after upgrading to 1.16.x. A warning about outdated
indexes will be issued on startup.

Indexes can be migrated to the new format with the `rethinkdb index-rebuild`
utility. Consult the [troubleshooting document][outdated-indexes-documentation] for more information.

[outdated-indexes-documentation]: http://rethinkdb.com/docs/troubleshooting#my-secondary-index-is-outdated

The abstraction of datacenters has been replaced by [server tags][server-tags-documentation].
Existing datacenter assignments will be converted to server tags automatically.

[server-tags-documentation]: http://rethinkdb.com/docs/sharding-and-replication/

### API-breaking changes ###

The `tableCreate`, `tableDrop`, `dbCreate` and `dbDrop` terms have a new set of return values.
The previous return values `created` and `dropped` have been renamed to `tables_creates` / `dbs_created` and
`tables_dropped` / `dbs_dropped` respectively. The terms now additionally return a `config_changes` field.

Consult the API documentation for these commands for details:
[tableCreate][tableCreate-api-docs], [tableDrop][tableDrop-api-docs], [dbCreate][dbCreate-api-docs], [dbDrop][dbDrop-api-docs]

[tableCreate-api-docs]: http://rethinkdb.com/api/javascript/tableCreate
[tableDrop-api-docs]: http://rethinkdb.com/api/javascript/tableDrop
[dbCreate-api-docs]: http://rethinkdb.com/api/javascript/dbCreate
[dbDrop-api-docs]: http://rethinkdb.com/api/javascript/dbDrop

Changefeeds on a table now combine multiple changes to the same document into a single notification if they happen rapidly.
You can turn this behavior off by passing the `squash: false` optional argument (see the
[API documentation][changes-api-docs] for details).

[changes-api-docs]: http://rethinkdb.com/api/javascript/changes

Strings passed to ReQL are now rejected if they are not valid UTF-8.
Non UTF-8 conformant data can be stored as [binary][binary-api-docs] data instead.

[binary-api-docs]: http://rethinkdb.com/api/javascript/binary/


## New features ##

* ReQL admin: a new cluster management and monitoring API (#2169)
  * Added a special system database named `rethinkdb`
    * The `table_config` table allows changing a table's configuration (#2870)
    * The `server_config` table allows listing and managing servers in the cluster (#2873)
    * The `db_config` table allows listing and renaming databases (#151, #2871)
    * The `cluster_config` table contains cluster-wide settings (#2924)
    * The `table_status` table displays each table's availability and replication status (#2983, #3269)
    * The `server_status` table contains information about the servers in the cluster (#2923, #3270)
    * The `current_issues` table lists issues affecting the cluster and suggests solutions (#2864, #3258)
    * The `jobs` table lists running queries and background tasks (#3115)
    * The `stats` table contains real-time statistics about the server (#2885)
    * The `logs` table provides access to the server logs (#2884)
    * The `identifierFormat` optional argument to `table` switches how databases, tables and servers are referenced in the system tables. Can be either "name" or "uuid" (#3266)
    * Added hidden debug tables (`_debug_table_status`, `_debug_stats`). These tables are subject to change and not part of the official administration interface (#2901, #3385)
  * Improved cluster management
    * Servers can now be associated with multiple tags using the `--server-tag` flag or by updating the `server_config` table (#2856)
    * Removed the datacenter abstraction and changed the arguments to `tableCreate` (#2876)
    * Removed the `rethinkdb admin` command line interface
    * Added a `reconfigure` command to change the replication and sharding settings of a table (#2932)
    * Added a `rebalance` command to even out the size of a table's shards (#2981)
    * Added a `config` command for tables and databases as an alias for the corresponding row in `db_config` or `table_config`
    * Added a `status` command for tables as an alias for the corresponding row in `table_status`
    * Most of the `/ajax` endpoints have been removed. Their functionality has been moved to the system tables (#2878, #2879)
    * The stats now contains the number of client connections (#2989)
    * Added more information to the return value of `db_create`, `db_drop`, `table_create` and `table_drop` (#3001)
    * Changed how `durability` and `write_acks` are configured (#3066)
    * The cache size can now be changed at runtime (#3166)
    * Improved the scalability for large table creation and reconfiguration in large clusters (#3198)
    * Added a new UI for table configuration (#3229)
    * Empty tables can now be sharded (#3271, #1679)
* ReQL
  * Added `r.range` which generates all numbers from a given range (#875)
  * Enforce UTF-8 encoding on input strings (#1181)
  * Added a `wait` command which waits for a table to be ready (#2259)
  * Added `toJsonString` which converts a datum into a JSON string (#2513)
  * Turned `map` into a variadic function for mapping over multiple sequences in parallel (#2574)
  * Added a prefix version of `map` (#3321)
  * Added an optional `squash` argument to the `changes` command, which lets the server combine multiple changes to the same document (defaults to `true`) (#2726, #3558)
  * `min` and `max` now use an index when passed the new `index` parameter (#2974, #2981)
  * It is now possible to get changefeeds for more types of queries. `changes` can now be chained onto:
    * Ranges generated with `between` (#3232)
    * Single documents with `get`
    * Subsets with `filter`
    * Sub-documents and modified documents with `map` and other commands that are stream-polymorphic such as `merge`
    * Certain reductions such as `min` and `max`
    * Top scoring documents with `orderBy` and `limit`
    * Combinations of the above, such as `between` followed by `map`
* Server
  * Made buffered IO the default, added a `--direct-io` flag to enable direct IO and deprecated the `--no-direct-io` flag (#3391)
* Python driver
  * `rethinkdb export` now exports secondary index information and `rethinkdb import` re-creates those indexes unless it is given the `--no-secondary-indexes` flag (#3484)

## Improvements ##

* Web UI
  * The web assets are now compiled into the `rethinkdb` executable (#1093)
  * `getAll` queries on secondary indexes are now shown in the performance graph (#2379)
  * Added `getField` to the autocompletion in the Data Explorer (#2624)
  * Added live updates for changefeeds in the Data Explorer (#2643)
  * Reduced the amount of data transferred to and processed by the browser by the dashboard (#2786)
  * The database view was removed (#3491)
  * Added a secondary index monitor to the dashboard (#3493)
* Server
  * Removed code used to support outdated tests (#1524)
  * Improved implementation of erase range operations (#2034)
  * `jemalloc` is now used by default instead of `tcmalloc`, which solves some memory inflation (#2279)
  * Replaced vector clocks with automatic timestamp-based conflict resolution (#2784)
  * No longer complain when `stderr` can't be flushed (#2790)
  * Replaced uses of the word "machine" to use "server" instead. The old `--machine-name` flag is now `--server-name` (#2877, #3254)
  * The server now prints its own name on startup (#2992)
  * Adjusted the formatting of log levels, no longer print `info:` lines to stderr and added a `notice` log level (#3040)
  * Added a `--no-default-bind` option that prevents the server from listening on loopback addresses (#3154)
  * Lower the CPU load of an idle server (#3185)
  * The OS disk cache is now ignored when calculating available memory (#3193)
  * `kqueue` is now used instead of `poll` for events on OS X for better performance (#3403)
  * Tables now become available faster and table metadata takes less space on disk (#3463)
  * Queries that perform multiple HTTP requests now share the same cookies (#3469)
  * Made resolving of ties in secondary indexes more consistent (#3550)
  * The server now calls home to RethinkDB HQ to check for newer versions and send anonymized statistics. This can be turned off with the new `--no-update-check` flag (#3170)
* Testing
  * The ReQL tests can now run in parallel (#2305, #2672)
  * The Ruby driver is now tested against different versions of Ruby (#2526)
  * Relaxing floating point comparison to account for rounding issues (#2912)
  * No longer depend on external HTTP servers (#3047)
  * Switched to using the new cluster management features in the test suite (#3398, #3401)
* ReQL
  * Allow querying the version of the server (#2698)
  * Improved the error message when pseudo-types are used as objects (#2766)
  * The names of types returned by `typeOf` and `info` now match (#3059)
  * No longer silently ignore unknown global optional arguments (#2052)
  * Improved handling of socket, protocol and cursor errors (#3207)
  * Added an `isOpen` method to connections (#2113)
  * `info` on tables now works when `useOutdated` is true (#3355)
  * `info` on tables now includes the table id (#3358)
* Driver protocol
  * Made internal names of commands more consistent (#3147)
  * Added the `SUCCESS_ATOM_FEED` return type, used when returning changes for a single document (#3389)
* JavaScript driver
  * Upgrade to bluebird 2 (#2973)
* Python driver
  * Added a `next` method for cursors (#3042)
  * `r.expr` now accepts any iterable or mapping (#3146)
  * `rethinkdb export` now avoids traversing each table twice by using an estimated document count (#3483)
* Build
  * Updated code to build with post-Yosemite versions of Xcode (#3219)
  * Fetch Curl 7.40.0 when not using the system-installed one (#3540)
* Packaging
  * Added `procps` as a dependency in the Debian packages (#3404)
  * Now depend on `jemalloc` instead of `gperftools` (#3419)

## Bug fixes ##

* Tests
  * Fixed the `TimerTest` test (#549)
  * Always use the in-tree driver for testing (#711)
* Server
  * Some startup log messages are now consolidated into a single log entry (#925)
  * Fixed continuation of partially-interrupted vectored I/O (#2665)
  * Fixed an issue in the log file parser (#3694)
* ReQL
  * Changed type of `between` queries to from `TABLE` to the more correct `TABLE_SLICE` (#1728)
* Web UI
  * Use a real fixed width font for backtraces to make them align correctly (#2065)
  * No longer show empty batches when more data is available (#3432)
* Python driver
  * Correctly handle Unicode characters in error messages (#3051)
* JavaScript driver
  * Added support for passing batch configuration arguments to `run` (#3161)
* Ruby driver
  * Optional arguments can now be passed to `order_by` (#3221)

## Contributors ##

Many thanks to external contributors from the RethinkDB community for helping
us ship RethinkDB 1.16. In no particular order:

* Adam Grandquist (@grandquista)
* Ilya Radchenko (@knownasilya)
* Gianluca Ciccarelli (@sturmer)
* Alexis Okuwa (@wojons)
* Mike Ma (@cosql)
* Patrick Stapleton (@gdi2290)
* Mike Marcacci (@mike-marcacci)
* Andrei Horak (@linkyndy)
* Param Aggarwal (@paramaggarwal)
* Brandon Zylstra (@brandondrew)
* Ed Costello (@epc)
* Alessio Basso (@alexdown)
* Benjamin Goodger (@goodgerster)
* Vinh Quốc Nguyễn (@kureikain)

--

# Release 1.15.3 (Lawrence of Arabia)

Released on 2015-01-08

Bug fix update.

* Fixed a bug that caused the endpoints of a reversed range to not be
  correctly included or excluded (#3449)
* Fixed the `reverse_iterator` implementation for leaf nodes (#3446)
* Fixed a bug that could cause a bad ordering of secondary indexes for
  rows with different primary key sizes (#3444)
* Fixed a bug that could cause a crash under high load when using
  changefeeds (#3393)
* Fixed a bug that made it impossible to chain `between` and
  `distinct` (#3346)
* Changed some calls to avoid passing `NULL` to `memcpy` (#3317)
* Fixed the installer artwork on OS X Yosemite (#3253)
* Changed the version scheme for the JavaScript driver to avoid
  mis-use of pre-release numbers (#3281)
* Fixed a bug that could cause `rethinkdb import` and `rethinkdb
  export` to hang (#3458)

---

# Release 1.15.2 (Lawrence of Arabia)

Released on 2014-11-07

Bug fix update.

* Added packages for Ubuntu 14.10 "Utopic Unicorn" (#3237)
* Fixed a bug with memory handling in S2 (#3201)
* Fixed a bug handling paged results in the Data Explorer (#3111)
* Fixed a bug that caused a crash on exit if a joined server with an open
  changefeed crashed (#3038)
* Fixed a bug that caused a crash when unsharding discarded more rows than
  expected when batching results (#3264)
* Fixed a bug that could lead to crashes when changefeeds were frequently
  registered and unregistered (#3205)
* Changed the `r.point` constructor to be deterministic, allowing it to be used
  in secondary index functions (#3287)
* Fixed an incompatibility problem between Python 3.4 and the `import` command
  (#3259)
* Fixed a buffer alignment issue with `object_buffer_t` data (#3300)

---

# Release 1.15.1 (Lawrence of Arabia)

Released on 2014-10-07

Bug fix update.

* Fixed a bug where tables were always created with hard durability, regardless
  of the `durability` option (#3128)
* Fixed a bug that caused HTTPS access with `r.http` to fail under OS X (#3112)
* Fixed a bug in the Python driver that caused pickling/unpickling of time
  objects to fail (#3024)
* Changed the Data Explorer autocomplete to not override Ctrl+Tab on Firefox
  (#2959)
* Fixed a bug that caused a crash when a non-directory file was specified as
  RethinkDB's startup directory (#3036)
* Added native packages for Debian (#3125, #3107)
* Fixed a compilation error on ARM CPUs (#3116)
* Support building with Protobuf 2.6.0 (#3137)

---

# Release 1.15.0 (Lawrence of Arabia)

Released on 2014-09-23

The highlights of this release are support for geospatial objects and queries,
and significant performance upgrades relating to datum serialization (twice as
fast for many analytical workloads). Read the [release blog post][1.15-blog]
for more details.

[1.15-blog]: http://rethinkdb.com/blog/1.15-release/

Only documents modified after upgrading to 1.15 will receive these performance
gains. You may "upgrade" older documents by performing any write that modifies
their contents. For example, you could add a dummy field to all the documents in
a table and then remove it:

    r.table('tablename').update({dummy_field: true})
    r.table('tablename').replace(r.row.without('dummy_field'))

There are no API-breaking changes in this release.

## New features ##

* ReQL
  * Added geospatial query and index support (#2571, #2847, #2851, #2854, #2859,
    #3003, #3011)
  * Added `r.uuid` for generating unique IDs (#2063)
  * Added a `BRACKET` term to the query language, to improve the bracket
    operator in client drivers (#1179)

## Improvements ##

* Server
  * Significantly improved performance of read operations by lazily
    deserializing data: ~1.15x faster for simple queries, ~2x faster for many
    analytical queries, and ~50x for count queries (#1915, #2244, #2652)
  * Removed the option for `datum_t` to be uninitialized (#2985)
  * Improved the performance of `zip` by replacing the `zip_datum_stream_t` type
    with a transformer function (#2654)
  * Clarified error messages when the data in the selection could not be printed
    (#972)
  * Improved performance of `r.match` by adding regex caching and a framework
    for generic query-based caches (#2196)
* Testing
  * Removed unnecessary files from `test/common` (#2829)
  * Changed all tests to run with `--cache-size` parameter (#2816)
* Python driver
  * Modified `r.row` to provide an error message on an attempt to call it like a
    function (#2960)
* JavaScript driver
  * Errors thrown by the driver now have a stack trace (#3087)

## Fixed bugs ##

* ReQL
  * Fixed a bug for `r.literal` corner cases (#2710)
  * Improved error message when `r.literal` is used in an invalid context
    (#1600)
* Web UI
  * Fixed a bug that caused selection in the query text area to become
    unresponsive with large queries (#3043)
  * Fixed a bug that caused "more data is available" to be displayed incorrectly
    in certain cases (#3037)
* Server
  * Fixed a display bug with log entries in the web UI (#2627)
  * Fixed a bug where Makefile miscounted dependencies when `ql2.proto` was
    changed (#2965)
  * Fixed a bug where the connection authorization key was improperly encoded
    (#2952)
  * Fixed an uninitialized variable warning during builds (#2977)
* Testing
  * Fixed various bugs in tests (#2940, #2887, #2844, #2837, #2603, #2793)
* JavaScript driver
  * Fixed a bug in the JavaScript driver that caused backtraces to not print
    properly (#2793)
* Python driver
  * Replaced `or isinstance` with a tuple of types (#2968)
  * Removed unused `kwarg` assignments (#2969)
* Ruby driver
  * Fixed a bug where `default_db`, `host` and `port` were not exposed in the
    Connection object (#2849)

## Contributors ##

Many thanks to external contributors from the RethinkDB community for helping
us ship RethinkDB 1.15. In no particular order:

* Sathyanarayanan Gunasekaran (@gsathya)
* Adam Grandquist (@grandquista)
* Duane Johnson (@canadaduane)
* Colin Mattson(@cmattson)
* Justas Brazauskas (@jutaz)
* Matt Stith (@stith)
* Dmitry Minkovsky (@dminkovsky)

---

# Release 1.14.1 (Brazil)

Released on 2014-09-09

Bug fix update.

* Fixed a bug that caused `rethinkdb index-rebuild` to fail with auth keys (#2970)
* Changed `rethinkdb export` to specify `binary_format='raw'` to work with binary data correctly (#2964)
* Fixed `rethinkdb import` to handle Unicode in CSV files (#2963)
* Updated the Valgrind suppressions file to fix false "uninitialized value" warnings (#2961)
* Fixed a bug that caused duplicate perfmons when recreating an index (#2951)
* Fixed a bug that could cause `r.http` to crash when used with pagination and `coerce_to` (#2947)
* Improved the printing of binary data in Python and Ruby REPLs (#2942)
* Fixed a bug that could corrupt databases larger than 4GB on 32-bit systems (#2928)
* Fixed permission issues with the web admin interface files (#2927)
* Fixed a bug that caused a crash due to incorrect error handling in profile mode (#2718)
* Fixed a bug that caused a crash when existing servers tried to connect to a new server with an unresolvable hostname (#2708)

---

# Release 1.14.0 (Brazil)

Released on 2014-08-20

The highlights of this release are:

* Support for storing binary data
* Seamless database migration
* Support for Python 3

Read the [release blog post][1.14-blog] for more details.

[1.14-blog]: http://rethinkdb.com/blog/1.14-release/

## Compatibility ##

### Backwards-compatible changes ###

Data files from RethinkDB versions 1.13.0 onward will be automatically
migrated to version 1.14.x. As with any major release, back up your data files
before performing the upgrade. If you are upgrading from a release earlier
than 1.13.0, follow the [migration instructions][1.14-migration] before upgrading:

[1.14-migration]: http://rethinkdb.com/docs/migration/

Secondary indexes now use a new format; old indexes will continue to work, but
you should rebuild indexes after upgrading to 1.14.x. A warning about outdated
indexes will be issued on startup.

Indexes can be migrated to the new format with the `rethinkdb index-rebuild`
utility. Consult the [troubleshooting document][1.14-outdated-index] for more
information:

[1.14-outdated-index]: http://rethinkdb.com/docs/troubleshooting/#my-secondary-index-is-outdated

### API-breaking changes ###

The `return_vals` optional argument for `insert`, `delete` and `update` has
been changed to `return_changes`, and works with all write operations
(previously, this only worked with single-document writes). The returned
object is in a new format that is backwards-incompatible with previous
versions. Consult the API documentation for [insert][1.14-insert], [delete][1.14-delete],
and [update][1.14-update] for details:

[1.14-insert]: http://rethinkdb.com/api/javascript/insert
[1.14-delete]: http://rethinkdb.com/api/javascript/delete
[1.14-update]: http://rethinkdb.com/api/javascript/update

The `upsert` optional argument to `insert` has been replaced with `conflict`
and new allowed values of `error`, `replace` or `update`. This is a
backwards-incompatible change. Consult the API documentation for more
information.

## New features ##

* Server
  * Return old/new values for multi-row write operations (#1382)
  * `upsert` replaced with `conflict` argument for `insert` (#1838)
  * Added binary data type support via `binary` (#137, #2612, #2931)
  * `binary_format="raw"` added to `run` (#2762)
  * Secondary indexes can be renamed with `index_rename` (#2794)
  * Secondary indexes can be duplicated (#2797)
  * Out of date secondary indexes logged on startup (#2798)
  * `r.http` can return a binary object (#2806)
* Python driver
  * Added Python 3 support (#2502)
* Data Explorer
  * Support for displaying binary types (#2804, #2865)

## Improvements ##

* Server
  * Server names now default to the hostname of the machine (#236, #2548)
  * `distinct` is faster and now works on indexes (#1864)
  * Improve secondary index queue handling (#1947)
  * The array limit is now configurable (#2059)
  * Allow initializing an empty directory (#2359)
  * Max number of extprocs raised (#2391, #2584)
  * Argument count errors are prettier (#2568)
  * Better error reporting in `r.js` (#2748)
  * `index_status` provides more info (#2791)
* Command line
  * Table names in the CLI are disambiguated (#2360, #2550)
* Python driver
  * Cleanup unneeded files (#2610)
  * Documentation examples are now PEP8 compliant (#2534)
* Testing
  * Polyglot targets for Ruby 1.8, 1.9, 2.0, 2.1 (#773)
  * Multiple versions of Python can be tested simultaneously
* Web UI
  * Added a notification and helpful message for out-of-date secondary indexes
    (#2799)

## Fixed bugs ##

* Server
  * `--runuser` and `--rungroup` set proper permissions when used with
    `rethinkdb create` (#1722)
  * Improved behavior and error messages for write acks (#2039)
  * Fix `getaddrinfo` error handling (#2110)
  * Fix a bug with machine name generation (#2552, #2569)
  * Fix a bug when compiling on GCC 4.7.2 (#2572)
  * Fix memory corruption error (#2589)
  * Fix stream cache error (#2607)
  * Fix linking issue with RE2 (#2685)
  * Miscellaneous build fixes (#2666, #2827)
  * Fix rare conflict bug in cluster config (#2738)
  * Fix variable initialization error (#2741)
  * Fix bug in `RPCSemilatticeTest.MetadataExchange` (#2758)
  * Changefeeds work on multiple joined servers (#2761)
  * Secondary index functions ignore global optargs (#2767)
  * Secondary indexes sort correctly (#2774, #2789)
  * Fix crashing bug with undefined ordering (#2777)
  * Fix dependency includes in Makefile (#2779)
  * Convert `query_params` to a `map` (#2812)
  * Convert `header_lines` into a `map` (#2818)
  * Improve robustness with big documents (#2832)
  * Wipe out old secondary index tree when post-constructing (#2925)
  * Preliminary fix for web permission issues on CentOS (#2927)
* ReQL
  * Fix a bug in `delete_at` (#2696)
  * `insert` and `splice` now check array size limit (#2697)
  * Fix error in undocumented `batch_conf` option (#2709)
* Web UI
  * Improve interval notation for shard boundaries (#2081, #2635)
  * The Data Explorer now remembers the current query on disconnects (#2460)
  * The Data Explorer now hides overflowing text in its popup containers
    (#2593)
* Testing
  * Miscellaneous testing improvements (#2396, #2583, #2760, #2787, #2788,
    #2795)
  * Update testing scripts for drivers (#2815)
* Python driver
  * Make `all` and `any` behave like `and_` and `or_` (#2659)
  * Use `os.path.splitext` to check file extensions (#2681)
* JavaScript driver
  * Return errors rather than throw them (#2852, #2883)

## Contributors ##

Many thanks to external contributors from the RethinkDB community for helping
us ship RethinkDB 1.14. In no particular order:

* Sathyanarayanan Gunasekaran (@gsathya)
* James Costian (@jamescostian)
* Ed Rooth (@sym3tri)
* Cole Gleason (@colegleason)
* Mikhail Goncharov (@metaflow)
* Elian Gidoni (@eliangidoni)
* Ivan Fraixedes (@ifraixedes)
* Brett Griffin (@brettgriffin)
* Ed Costello (@epc)
* Juuso Haavisto (@9uuso)
* Nick Verlinde (@npiv)
* Ayman Mackouly (@1N50MN14)
* Adam Grandquist (@grandquista)

---

# Release 1.13.4 (My Name is Nobody)

Released on 2014-08-14

Bug fix update.

* Resolved several memory leaks (#2899, #2840, #2744)
* Added a `--temp-dir` option to `rethinkdb dump` and `rethinkdb restore` (#2783)
* The `batchConf` argument to `run` now works in the JavaScript driver (#2707)
* Fixed the `Accept-Encoding` header sent by `r.http` (#2695)
* Improved the performance of inserting large objects from the JavaScript driver (#2641)
* `cursor.close` now takes a callback in the JavaScript driver (#2591)
* Fixed a bug that caused `illegal to destroy fifo_enforcer_sink_t` errors (#2264)
* Fixed a bug that caused `Assertion failed: [!parent->draining.is_pulsed()]` errors (#2811)
* Improved the error message when running into version incompatibilities (#2657)
* Improved the garbage collection for `r.js` (#2642)

---

# Release 1.13.3 (My Name is Nobody)

Released on 2014-07-07

Bug fix update.

* Fix a compiler warning: `cluster_version may be used uninitialized` (#2640)
* Fix code that used `std::move` twice on the same object (#2638)
* Prepare for live cluster upgrades (#2563)
* Fix a bug that could lead to inconsistent data (#2579)

---

# Release 1.13.1 (My Name is Nobody)

Released on 2014-06-26

Bug fix update.

* Fixed a bug that caused `Assertion failed: [ptr_]` errors when shutting down (#2594)
* Fixed a performance issue in the JSON parser (#2585)
* The JavaScript driver no longer buffers change feeds (#2582)
* Fixed a bug that caused `Uncaught exception of type "cannot_perform_query_exc_t"` errors (#2576)
* No longer crash when a secondary index is named `primary` (#2575)
* Queries that return `null` are now handled correctly in the Data Explorer (#2573)
* `r.http` now properly parses headers when following a redirection (#2556)
* Improved the performance of write operations on sharded tables (#2551)
* Fixed a bug that caused `r.js` to crash in certain circumstances (#2435)
* Correctly handle `EPIPE` errors when connecting to an old version of the server (#2422)
* Fixed a bug that caused `Could not bind socket` errors when using `--bind` (#2405)
* Fixed a bug that caused `Failed to parse  as valid uuid` errors (#2401)
* Improved the `bad magic number` error message (#2302)
* `default` now catches index out of bounds errors on streams (#1922)
* Improved arity error messages in the JavaScript driver (#2449)

---

# Release 1.13.0 (My Name is Nobody)

Released on 2014-06-13

The highlights of this release are the `r.http` command for external
data access, change feed support via the new `changes` command, and
full support for Node.js promises in the JavaScript driver.

Read the [release blog post][1.13-blog] for more details.

[1.13-blog]: http://rethinkdb.com/blog/1.13-release/

## Compatibility ##

This release is not compatible with data files from earlier
releases. If you have data you want to migrate from an older version
of RethinkDB, please follow the [migration instructions][1.13-migration] before
upgrading:

[1.13-migration]: http://rethinkdb.com/docs/migration/

There are also some backwards incompatible changes in the JavaScript driver.

* The `hasNext` command for cursors has been removed. `next` can be used instead.

## New features ##

* ReQL
  * Added `r.random` for generating random numbers (#865)
  * Made the second argument to `slice` optional ([#2048](http://gabrielecirulli.github.io/2048/))
  * `eq_join` now accepts a function as its first argument, and does not fail if the field doesn't exist (#1764)
  * `nth` can now return a selection, just like `get` (#348)
  * Improved the `master not available` error message (#1811)
  * Switched to the JSON protocol in the Ruby, JavaScript and Python drivers (#2224, #2390)
  * Added the `changes` command for creating live change feeds (#997)
  * Added `r.args` to allow specifying a dynamic number of arguments to commands such as `get_all` (#1854)
  * Added `r.http` for interfacing with external APIs (#1383)
* Server
  * Added a JSON protocol to replace the protobuf protocol, which is now deprecated (#1868)
  * Added a README describing the structure of the `src/` folder (#2301)
  * Switched to manual versioning of the intra-cluster protocol (#2295)
  * Made the serialization format version-aware (#2308, #2353)
  * Improved the error message when running out of disk space (#1945)
* JavaScript driver
  * Added support for promises (using bluebird) (#1395)
  * Removed the `hasNext` command (#2497)
  * Added the `on`, `once`, `removeListener` and `removeAllListeners` methods to cursors (#2223)
  * The first argument to `r.connect` has been made optional (#2273)
* Tests
  * Improved the run-test script and ported it to Python (#2235)
  * Improved the ReQL tests (#1402)
* Build
  * Symbol files are now generated (#2330)
  * Build with debugging symbols by default (#2323)
  * Added a signature to the OS X package (#1565)
  * Dropped support for GCC 4.4 (#1748)
  * Added an explicit dependency on Python 2 in the `configure` script (#2478)
  * Added a dependency on libcurl (#2400)

## Improved performance ##

* Server
  * Allocate smaller pages in the cache (#2130)
  * Reduce overhead by handling requests locally on the primary if possible (#2083)
  * Adjusted the value of `chunk_processing_semaphore` (#2392)
  * Improved backfilling on rotational drives (#2393)
  * Metadata is no longer copied when evaluating `r.db` (#1907)
  * No longer update the stat block when updating secondary indexes (#2431)
  * Block writes are better combined in the cache (#2127)
  * Concurrent garbage collection to improve disk space efficiency (#2457)
* Testing
  * Added automated performance regression tests (#1963)

## Fixed bugs ##

* Server
  * Fixed the threaded coroutine implementation (#2168, #2332)
  * HTTP 500 errors are now accompanied by an error message (#511)
  * Got rid of vestigial memcache support (#803)
  * Made `order_by` and other sortings be stable (#2155)
  * Cleaned up blob_t code to make it more reliable (#2227)
  * Fixed a bug that caused crashes when dropping secondary indexes under load (#2251)
  * Fixed a bug in the JSON parsing code that caused a crash (#2489)
  * Fixed a bug that could cause segfaults (#2491)
  * Avoid high memory consumption on startup (#2329)
  * Disabled Nagle algorithm for outgoing TCP connections (#2529)
  * Remove some potentially objectionable server names (#2468)
  * Fixed a bug that caused `Callstack overflow in a coroutine` errors (#2357)
  * Merged upstream fixes to `cJSON` (#2496)
  * Fixed a bug that could cause a segmentation fault (#2500)
  * Fixed a bug in the serializer garbage byte calculation (#2541)
* ReQL
  * Added the database name to error messages (#2280)
  * No longer report run-time errors as client errors (#1908)
  * Arguments to `r.expr` are now properly validated (#2384)
  * No longer crash when `r.js` returns a bad datum (#2409)
  * Fixed handling of global optargs (#2525)
* Ruby driver
  * Ignore `close` errors when reconnecting (#2276)
  * Fixed conflicts with active support (#2284)
  * Added a missing `nesting_depth` argument to `r.expr` (#2408)
  * Modified the driver to work with JRuby (#2296)
  * The driver now prefetches cursor data (#2373)
* JavaScript driver
  * Improved the variable names in error messages (#2461)
* Web UI
  * Fixed a bug that caused JavaScript exceptions (#2503)
  * Fixed the per-server document count (#1836)
  * The database name is now shown on the table page (#2366)
  * Removed the inconsistent green tick next to the secondary index status (#2084)
  * Fixed email highlighting (#2378)
  * Large responses no longer cause the Data Explorer to become unresponsive (#2481)
  * Fixed a bug triggered by clearing the Data Explorer history (#2389)
* Tests
  * Converted the memcache tests to use ReQL (#803)

## Contributors ##

Many thanks to external contributors from the RethinkDB community for
helping us ship RethinkDB 1.13. In no particular order:

* Liu Aleaxander (@Aleaxander)
* Nicolas Viennot (@nviennot)
* Elian Gidoni (@eliangidoni)
* Matthew Frazier (@leafstorm)
* Masatoshi Ishida (@Masatoshi)

---

# Release 1.12.5 (The Wizard of Oz)

Released on 2014-05-21

Bug fix update.

* Fixed a bug that caused `Guarantee failed: [!mod_info->deleted.second.empty() && mod_info->added.second.empty()]` errors (#2285)
* Fixed the behaviour of `order_by` following `between` (#2307)
* Fixed a bug that caused `Deserialization of rdb value failed with error archive_result_t::RANGE_ERROR` errors (#2399)
* JavaScript driver: `reduce` no longer accepts the `base` argument (#2288)
* Python driver: improved the error message when a cursor's connection is closed (#2291)
* Python driver: improved the implementation of cursors (#2364, #2337)

---

# Release 1.12.4 (The Wizard of Oz)

Released on 2014-04-22

Bug fix update.

* Fixed a bug that caused `Assertion failed: [page->is_disk_backed()]` errors (#2260)
* Fixed a bug that caused incorrect query results and frequent server crashes under low memory conditions (#2237)

---

# Release 1.12.3 (The Wizard of Oz)

Released on 2014-04-17

Bug fix update.

* Compilation no longer fails with SEMANTIC_SERIALIZER_CHECK=1 (#2239)
* Identical `--join` options no longer cause a crash (#2219)
* Fixed a race condition in the Ruby driver (#2194)
* Concurrent backfills are now limited to 4 per peer and 12 total (#2211)
* Failure to `fsync` a directory is now a warning instead of an error (#2255)
* Packages are now available for Ubuntu 14.04, codename Trusty Tahr (#2101)

---

# Release 1.12.2 (The Wizard of Oz)

Released on 2014-04-08

Bug fix update.

* Fixed a bug that caused `illegal to destroy fifo_enforcer_sink_t` errors (#2092)
* Fixed a bug that caused `Guarantee failed: [resp != __null]` errors (#2214)
* Fixed a bug that caused `Segmentation fault` errors (#2222)
* Use less memory for common operations (#2164, #2213)
* Ruby 2.1.1 support (#2177)
* Fixed the `PageTest` unit test (#2187)
* Updated the documentation (#2197)
* Updated the sample config file (#2205)

---

# Release 1.12.1 (The Wizard of Oz)

Released on 2014-03-27

Bug fix update.

* Fixed crash `evicter.cc at line 124: Guarantee failed: [initialized_]` (#2182)
* Fixed `index_wait` which did not always work (#2170, #2179)
* Fixed a segmentation fault (#2178)
* Added a `--hard-durability` option to import/restore
* Changed the default `--cache-size` to be more friendly towards machines with less free RAM
* Changed tables to scale their soft durability throttling based on their cache size
* Fixed some build failures (#2183, #2174)
* Fixed the Centos i686 packages (#2176)

---

# Release 1.12.0 (The Wizard of Oz)

Released on 2014-03-26

The highlights of this release are a simplified map/reduce, an
experimental ARM port, and a new caching infrastructure.

Read the [release blog post][1.12-blog] for more details.

[1.12-blog]: http://rethinkdb.com/blog/1.12-release/

## Compatibility ##

This release is not compatible with data files from earlier
releases. If you have data you want to migrate from an older version
of RethinkDB, please follow the [migration instructions][1.12-migration] before
upgrading:

[1.12-migration]: http://rethinkdb.com/docs/migration/

There are also some backwards incompatible changes in ReQL, the query
language.

* `grouped_map_reduce` and `group_by` were replaced by `group`
* `reduce` no longer takes a `base` argument; use `default` instead
* `table_create` no longer takes a `cache_size` argument
* In JavaScript, the calling convention for `run` has changed.

## New features ##

* Server
  * Added support for the ARM architecture (#1625)
  * Added per-instance cache quota and removed per-table quotas (#97)
    * Added a `--cache-size` command line option that sets the cache size in MiB for a single instance
    * Removed the `cache_size` optional argument for `table_create`
  * Wrote a new and improved cache (#1642)
  * Added gzip compression to the built-in web server (#1746)
* ReQL
  * `merge` now accepts functions as an argument (#1345)
  * Added an `object` command to build objects from dynamic field names (#1857)
  * Removed the optional `base` argument to `reduce` (#888)
  * Added a `split` command to split strings (#1099)
  * Added `upcase` and `downcase` commands to change the case of a string (#874)
  * Change how aggregation and grouping is performed (#1096)
    * Removed `grouped_map_reduce` and `groupBy`
    * Added `group` and `ungroup`
    * Changed the behavior of `count`, `sum` and `avg` and added `max` and `min`
* Web UI
  * Display index status and progress bars in the web UI (#1614)

## Improved performance ##

* Server
  * Improved the scalability of meta operations (such as table creation) to allow for more nodes in a cluster (#1648)
  * Changed the CPU sharding factor to 8 for improved multi-core scalability (#1043)
  * Batch sizes now scale better, which speeds up operations like `limit` (#1786)
  * Tweaked batch sizes to avoid computing unused results (#1962)
  * Improved throttling and LBA garbage collection to avoid stalls in query throughput (#1820)
  * Many optimizations to avoid having the web UI time out when the server is under load (#1183)
  * Improved backfill performance by using batches for sending and inserting data (#1971)
  * Reduced wasteful copies when serializing (#1431)
  * Evaluating queries now yields occasionally to avoid timeouts (#1631)
  * Reduced performance impact of backfilling on other queries (#2071)
* Web UI
  * Improved how the web UI handles large clusters with many tables (#1662)
* Ruby driver
  * Removed inefficient construction-location backtraces (#1843)
* Tests
  * Added automated performance tests (#1806)

## Fixed bugs ##

* Server
  * Improved the code to avoid heartbeat timeout errors when resharding (#1708)
  * Resharding one table no longer makes other tables unavailable (#1751)
  * Improved the blueprint suggester to distribute shards more evenly (#344)
  * Queries from the data explorer are now interruptible (#1888)
  * No longer fail if IPv6 is not available (#1925)
  * Added support for the new v8 scope API (#1510)
  * Coroutine stacks now freed to reduce memory consumption (#1670)
  * Added range get, backfill and secondary index reads to the stats (#660)
  * `r.table(...).count()` no longer stalls inserts (#1870)
  * Fixed crashes when adding a secondary index (#1621, #1437)
  * Improve the handling of out-of-memory conditions (#2003)
  * Secondary index construction is now reported as a write operation in the stats (#1953)
  * Fixed a crash that occasionally happened during replication (#1389)
  * `linux_file_t::set_size` no longer makes blocking syscalls (#265)
  * Fixed a crash caught by the unit tests (#1084)
* Command line
  * `rethinkdb admin` now prints warnings to stderr instead of stdout (#1316)
  * The `import` and `export` scripts now display a row count when done (#1659)
  * Added support for `log_file` and `no_direct_io` in the init script (#1769, #1892)
  * Do not display a stack trace for regular errors printed by the backup scripts (#2098)
  * `rethinkdb export` no longer fails when there are no tables (#1904)
  * `rethinkdb import` no longer tries to parse CSV files as JSON (#2097)
* Web UI
  * No longer display wrong number of rows when a string is returned (#1669)
  * The data explorer now properly closes cursors (#1569)
  * The data explorer now displays empty tables correctly (#1698)
  * Links are now relative so they can be proxied from a subdirectory (#1791)
  * The server time reported by the profiler is now accurate (#1784)
  * Improved the flow for removing dead servers (#1366)
  * No longer lower replicas to 0 if the datacenter is primary (#1834)
  * Now displays consistent availability information (#1756)
  * Fixed a XSS security issue (#2018)
  * Changing the number of acks no longer displays the sharding bar (#2023)
  * The backfilling progress bar doesn't disappear when refreshing the page (#1997)
  * Correctly handle newlines in the data explorer (#2021)
  * Sort the table list in alphabetical order (#1704)
  * Added mouseover text to the query execution time (#1940)
  * The replication status no longer blinks (#2019)
  * Fix inconsistencies in dates caused by DST (#2047)
* Ruby driver
  * Added a missing `close` method on cursors (#1568)
  * Improved conflict handling (#1814)
  * Use `define_method` instead of `method_missing`, which improves compatibility with Sinatra (#1896)
* Python driver
  * Improved the quality of the generated documentation (#1729)
  * Added missing `and_` and `or_` and a warning for misuse of `|` and `&` (#1582)
  * Added support for `r.row` in `eq_join` (#1810)
  * Added a detailed error message when brackets are not used properly (#1434)
  * `count` with an argument now behaves correctly (#1992)
  * Added missing `get_field` command (#1941)
* JavaScript driver
  * Added support for `r.row` in `eqJoin` (#1810)
  * Timeout events on the socket are now handled correctly (#1909)
  * No longer use the deprecated ArrayBuffer API (#1803)
  * Fix backtraces for optional arguments (#1935)
  * Changed the syntax of `run` (#1890)
  * Exposed the error constructors (#1926)
  * Fixed the string representation of functions (#1919, #1894)
  * Backtrace printing now works correctly for both protobufjs and node-protobuf (#1879)
  * No longer extend `Array.prototype` (#2112)
* Build
  * `./configure` now checks for `boost` and can fetch it if it is not installed (#1699)
  * The source distribution now includes the v8 source (#1844)
  * Use Python 2 to build v8 (#1873)
  * Improved how the build system fetches and builds external packages, and changed the default to not fetch anything (#1231)

## Packaging ##

* Support for Ubuntu Raring has been dropped (#1924)

---

# Release 1.11.3 (Breakfast at Tiffany's)

Released on 2014-01-14

Bug fix update.

* Fixed a crash on multiple ctrl-c (#1848)
* Ruby driver: fixed mutable backtraces (#1846)
* Fixed a build failure on older versions of GCC (#1824)
* Added missing durability argument to the Javascript driver (#1821)
* Fixed a crash caused by changing the cluster configuration when there are unresolved issues (#1813)
* Fixed a bug triggered by using `order_by` followed by `count` (#1796)
* Tables can now be referenced by full name (e.g. `database.table`) in `rethinkdb admin` (#1795)
* Fixed a bug triggered by chaining multiple joins (#1793)
* Fixed a crash occasionally triggered by dropping an index (#1789)
* The init script now fails when given wrong arguments (#1779)
* RethinkDB now refuses to start if it cannot open the log file (#1778)
* Fixed a JavaScript error in the Web UI (#1754)
* Speed up count queries (#1733)
* Fix a bug with thread-local storage that caused a segfault on certain platforms (#1731)
* `get` of a non-existent document followed by `replace` now works as documented (#1570)

---

# Release 1.11.2 (Breakfast at Tiffany's)

Released on 2013-12-06

Bug fix update.

* Fixed a bug caused by suggesting `r.ISO8601` in the Data Explorer
* Cursor in the Data Explorer is restored when a command is selected via dropdown
* Fixed a bug where queries returning null in the Data Explorer could not be parsed (#1739)
* Always `fsync` parent directories to avoid data loss (#1703)
* Fixed IPv6 issues with link-local addresses (#1694)
* Add some support for Python 3 in the build scripts (#1709)

---

# Release 1.11.1 (Breakfast at Tiffany's)

Released on 2013-12-02

Bug fix update.

* Drivers no longer ignore the timeFormat flag (#1719)
* RethinkDB now correctly sets the `ResponseType` field in responses to `STOP` queries (#1715)
* Fixed a bug that caused RethinkDB to crash with a failed guarantee (#1691)

---

# Release 1.11.0 (Breakfast at Tiffany's)

Released on 2013-11-25

This release introduces a new query profiler, an overhauled streaming infrastructure,
and many enhancements that improve ease and robustness of operation.

Read the [release blog post][1.11-blog] for more details.

[1.11-blog]: http://rethinkdb.com/blog/1.11-release/

## New Features ##

* Server
  * Removed anaphoric macros (#806)
  * Changed the array size limit to 100,000 (#971)
  * The server now reads blobs off of disk simultaneously (#1296)
  * Improved the batch replace logic (#1468)
  * Added IPv6 support (#1469)
  * Reduced random disk seeks during write transactions (#1470)
  * Merged writebacks on the serializer level (#1471)
  * Streaming improvements: smarter batch sizes are sent to clients (#1543)
  * Reduced the impact of index creation on realtime performance (#1556)
  * Optimized insert performance (#1559)
  * Added support for sending data back as JSON, improving driver performance (#1571)
  * Backtraces now span coroutine boundaries (#1602)
* Command line
  * Added a progress bar to `rethinkdb import` and `rethinkdb export` (#1415)
* ReQL
  * Added query profiling, enabled by passing `profile=True` to `run` (#175)
  * Added a sync command that flushes soft writes made on a table (#1046)
  * Made it possible to wait for no-reply writes to complete (#1388)
    * Added a `wait` command
    * Added an optional argument to `close` and `reconnect`
  * Added `index_status` and `index_wait` commands (#1562)
* Python driver
  * Added documentation strings (#808)
  * Added streaming and performance improvements (#1371)
  * Changed the installation procedure for using the C++ protobuf implementation (#1394)
  * Improved the streaming logic (#1364)
* JavaScript driver
  * Changed the installation procedure for using the C++ protobuf implementation (#1172)
  * Improved the streaming logic (#1364)
* Packaging
  * Made the version more explicit in the OS X package (#1413)

## Fixed Bugs ##

* Server
  * No longer access `perfmon_collection_t` after destruction (#1497)
  * Fixed a bug that caused nodes to become unresponsize and added a coroutine profiler (#1516)
  * Made database files compatible between 32-bit and 64-bit versions (#1535)
  * No longer use four times more cache space (#1538)
  * Fix handling of errors in continue queries (#1619)
  * Fixed heartbeat timeout when deleting many tables at once (#1624)
  * Improved signal handling (#1630)
  * Reduced the load caused by using the Web UI on a large cluster (#1660)
* Command line
  * Made `rethinkdb export` more resilient to low-memory conditions (#1546)
  * Fixed a race condition in `rethinkdb import` (#1597)
* ReQL
  * Non-indexed `order_by` queries now return arrays (#1566)
  * Type system now includes selections on both arrays and streams (#1566)
  * Fixed wrong `inserted` result (#1547)
  * Fixed crash caused by using unusual strings in `r.js` (#1638)
  * Redefined batched stream semantics (includes a specific fix for the JavaScipt driver as well) (#1544)
  * `r.js` now works with time values (#1513)
* Python driver
  * Handle interrupted system calls (#1362)
  * Improved the error message when inserting dates with no timezone (#1509)
  * Made `RqlTzInfo` copyable (#1588)
* JavaScript driver
  * Fixed argument handling (#1555, #1577)
  * Fixed `ArrayResult` (#1578, #1584)
  * Fixed pretty-printing of `limit` (#1617)
* Web UI
  * Made it obvious when errors are JavaScript errors or database errors (#1293)
  * Improved the message displayed when there are more than 40 documents (#1307)
  * Fixed date formatting (#1596)
  * Fixed typo (#1668)
* Build
  * Fixed item counts in make (#1443)
  * Bump the fetched version of `gperftools` (#1594)
  * Fixed how `termcap` is handled by `./configure` (#1622)
  * Generate a better version number when compiling from a shallow clone (#1636)

---

# Release 1.10.1 (Von Ryan's Express)

Released on 2013-10-23

Bug fix update.

* Server
  * `r.js` no longer fails when interrupted too early (#1553)
  * Data for some `get_all` queries is no longer duplicated (#1541)
  * Fixed crash `Guarantee failed: [!token_pair->sindex_write_token.has()]` (#1380)
  * Fixed crash caused by rapid resharding (#728)
* ReQL
  * `pluck` in combination with `limit` now returns a stream when possible (#1502)
* Python driver
  * Fixed `undefined variable` error in `tzname()` (#1512)
* Ruby driver
  * Fixed error message when calling unbound function (#1336)
* Packaging
  * Added support for Ubuntu 13.10 (Saucy) (#1554)

# Release 1.10.0 (Von Ryan's Express)

Released on 2013-09-26

Read the [release blog post][1.10-blog] for more details.

[1.10-blog]: http://rethinkdb.com/blog/1.10-release/

## New Features ##

* Added multi-indexes (#933)
* Added selective history deletion to the Data Explorer (#1297)
* Made `filter` support the nested object syntax (#1325)
* Added `r.not_` to the Python driver (#1329)
* The backup scripts are now installed by the Python driver (#1355)
* Implemented variable-width object length encoding and other improvements to serialization (#1424)
* Lowered the I/O pool threads' stack size (#1425)
* Improved datum integer serialization (#1426)
* Added a thin wrapper struct `threadnum_t` instead of using raw ints for thread numbers (#1445)
* Re-enabled full perfmon capability (#1474)

## Fixed Bugs ##

* Fixed the output of `rethinkdb admin help` (#936, #1447)
* `make` no longer runs `./configure` (#979)
* Made the Python code mostly Python 3 compatible (still a work in progress) (#1050)
* The Data Explorer now correctly handles unexpected token errors (#1334)
* Improved the performance of JSON parsing (#1403)
* `batched_rget_stream_t` no longer uses an out-of-date interruptor (#1411)
* The missing `protobuf_implementation` variable was added to the JavaScript driver (#1416)
* Improved the error and help messages in the backup scripts (#1417)
* `r.json` is no longer incorrectly marked as non-deterministic (#1430)
* Fixed the import of `rethinkdb_pbcpp` in the Python driver (#1438)
* Made some stores go on threads other than just 0 through 3 (#1446)
* Fixed the error message when using `groupBy` with illegal pattern (#1460)
* `default` is no longer unprintable in Python (#1476)

---

# Release 1.9.0 (Kagemusha)

Released on 2013-09-09

Read the [release blog post][1.9-blog] for more details.

[1.9-blog]: http://rethinkdb.com/blog/1.9-release/

Bug fix update.

* Server
  * Fixed a potential crash caused by a coroutine switch in exception handlers (#153)
  * No longer duplicate documents in the secondary indexes (#752)
  * Removed unnecessary conversions between `datum_t` and `cJSON` (#1041)
  * No longer load documents from disk for count queries (#1295)
  * Changed extproc code to use `datum_t` instead of `cJSON_t` (#1326)
* ReQL
  * Use `Buffer` instead of `ArrayBuffer` in the JavaScript driver (#1244)
  * `orderBy` on an index no longer destroys the preceding `between` (#1333)
  * Fixed error in the python driver caused by multiple threads using lambdas (#1377)
  * Fixed Unicode handling in the Python driver (#1378)
* Web UI
  * No longer truncate server names (#1313)
* CLI
  * `rethinkdb import` now works with Python 2.6 (#1349)

---

# Release 1.8.1 (High Noon)

Released on 2013-08-21

Bug fix update.

* The shard suggester now works with times (#1335)
* The Python backup scripts no longer rely on `pip show` (#1331)
* `rethinkdb help` no longer uses a pager (#1315, #1308)
* The web UI now correctly positions `SVGRectElement` (#1314)
* Fixed a bug that caused a crash when using `filter` with a non-deterministic value (#1299)

---

# Release 1.8.0 (High Noon)

Released on 2013-08-14

This release introduces date and time support, a new syntax for querying nested objects and 8x improvement in disk usage.

Read the [release blog post][1.8-blog] for more details.

[1.8-blog]: http://rethinkdb.com/blog/1.8-release/

## New Features ##

* ReQL
  * `order_by` now accepts a function as an argument and can efficiently sort by index (#159, #1120, #1258)
  * `slice` and `between` are now half-open by default (#869)
    * The behaviour can be changed by setting the new optional `right_bound` argument to `closed`
      or by setting `left_bound` to `open`
  * `contains` can now be passed a predicate (#870)
  * `merge` is now deep by default (#872)
    * Introduced `literal` to merge flat values
  * `coerce_to` can now convert strings to numbers (#877)
  * Added support for times (#977)
    * `+`, `-`, `<`, `<=`, `>`, `>=`, `==` and `!=`: arithmetic and comparison
    * `during`: match a time with an interval
    * `in_timezone`: change the timezone offset
    * `date`, `time_of_day`, `timezone`, `year`, `month`, `day`, `weekday`, `hour`, `minute` and `second`: accessors
    * `time`, `epoch_time` and `iso8601`: constructors
    * `monday` to `sunday` and `january` to `december`: constants
    * `now`: current time
    * `to_iso8601`, `to_epoch_time`: conversion
  * Add the nested document syntax to functions other than `pluck` (#1094)
    * `without`, `group_by`, `with_fields` and `has_fields`
  * Remove Google Closure from the JavaScript driver (#1194)
    * It now depends on the `protobufjs` and `node-protobuf` libraries
* Server
  * Added a `--canonical-address HOST[:PORT]` command line option for connecting RethinkDB nodes across different networks (#486)
    * Two instances behind proxies can now be configured to connect to each other
  * Optimize space efficiency by allowing smaller block sizes (#939)
  * Added a `--no-direct-io` startup flag that turns off direct IO (#1051)
  * Rewrote the `extproc` code, making `r.js` interuptible and fixing many crashes (#1097, #1106)
  * Added support for V8 >= 3.19 (#1195)
  * Clear blobs when they are unused (#1286)
* Web UI
  * Use relative paths (#1053)
* Build
  * Add support for Emacs' `flymake-mode` (#1161)

## Fixed Bugs ##

* ReQL
  * Check the type of the callback passed to `next` and `each` in the JavaScript driver (#656)
  * Fixed how some backtraces are printed in the JavaScript driver (#973)
  * Coerce the port argument to a number in the Python driver (#1017)
  * Functions that are polymorphic on objects and sequences now only recurse one level deep (#1045)
    * Affects `pluck`, `has_fields`, `with_fields`, etc
  * In the JavaScript driver, no longer fail when requiring the module twice (#1047)
  * `r.union` now returns a stream when given two streams (#1081)
  * `r.db` can now be chained with `do` in the JavaScript driver (#1082)
  * Improve the error message when combining `for_each` and `return_vals` (#1104)
  * Fixed a bug causing the JavaScript driver to overflow the stack when given an object with circular references (#1133)
  * Don't leak internal functions in the JavaScript driver (#1164)
  * Fix the qurey printing in the Python driver (#1178)
  * Correctly depend on node >= 0.10 in the JavaScript driver (#1197)
* Server
  * Improved the error message when there is a version mismatch in data files (#521)
  * The `--no-http-admin` option now disables the check for the web assets folder (#1092)
  * No longer compile JavaScript expressions once per row (#1105)
  * Fixed a crash in the 32-bit version caused by not using 64-bit file offsets (#1129)
  * Fixed a crash caused by malformed json documents (#1132)
  * Fixed a crash caused by moving `func_t` betweek threads (#1157)
  * Improved the scheduling the coroutines that sometimes caused heartbeat timeouts (#1169)
  * Fixed `conflict_resolving_diskmgr_t` to suport files over 1TB (#1170)
  * Fixed a crash caused by disconnecting too fast (#1182)
  * Fixed the error message when `js_runner_t::call` times out (#1218)
  * Fixed a crash caused by serializing unintitialised values (#1219)
  * Fixed a bug that caused an assertion failure in `protob_server_t` (#1220)
  * Fixed a bug causing too many file descriptors to be open (#1225)
  * Fixed memory leaks reported by valgrind (#1233)
  * Fixed a crash triggered by the BTreeSindex test (#1237)
  * Fixed some problems with import performance, interruption, parsing, and error reporting (#1252)
  * RethinkDB proxy no longer crashes when interacting with the Web UI (#1276)
* Tests
  * Fixed the connectivity tests for OS X (#613)
  * Fix the python connection tests (#1110)
* Web UI
  * Provide a less scary error message when a request times out (#1074)
  * Provide proper suggestions in the presence of comments in the Data Explorer (#1214)
  * The `More data` link in the data explorer now works consistently (#1222)
* Documentation
  * Improved the documentaiton for `update` and `pluck` (#1141)
* Build
  * Fixed bugs that caused make to not rebuild certain files or build files it should not (#1162, #1215, #1216, #1229, #1230, #1272)

---

# Release 1.7.3 (Nights of Cabiria)

Released on 2013-07-19

Bug fix update.

* RethinkDB no longer fails to create the data directory when using `--daemon` (#1191)

---

# Release 1.7.2 (Nights of Cabiria)

Released on 2013-07-18

Bug fix update.

* Fixed `wire_func_t` serialization that caused inserts to fail (#1155)
* Fixed a bug in the JavaScript driver that caused asynchronous connections to fail (#1150)
* Removed `nice_crash` and `nice_guarantee` to improve error messages and logging (#1144)
* `rethinkdb import` now warns when unexpected files are found (#1143)
* `rethinkdb import` now correctly imports nested objects (#1142)
* Fixed the connection timeout in the JavaScript driver (#1140)
* Fixed `r.without` (#1139)
* Add a warning to `rethinkdb dump` about indexes and cluster config (#1137)
* Fixed the `debian/rules` makefile to properly build the rethinkdb-dbg package (#1130)
* Allow multiple instances with different port offsets in the init script (#1126)
* Fixed `make install` to not use `/dev/stdin` (#1125)
* Added missing files to the OS X uninstall script (#1123)
* Fixed the documentation for insert with the returnVals flag in JavaScript (#1122)
* No longer cache `index.html` in the web UI (#1117)
* The init script now waits for rethinkdb to stop before restarting (#1115)
* `rethinkdb` porcelain now removes the new directory if it fails (#1070)
* Added cooperative locking to the rethinkdb data directory to detect conflicts earlier (#1109)
* Improved the comments in the sample configuration file (#1078)
* Config file parsing now allows options that apply to other modes of rethinkdb (#1077)
* The init script now creates folders with the correct permissions (#1069)
* Client drivers now time out if the connection handshake takes too long (#1033)

---

# Release 1.7.1 (Nights of Cabiria)

Released on 2013-07-04

Bug fix update.

* Fixed a bug causing `rethinkdb import` to crash on single files
* Added options to `rethinkdb import` for custom CSV separators and no headers (#1112)

---

# Release 1.7.0 (Nights of Cabiria)

Released on 2013-07-03

This release introduces hot backup, atomic set and get operations, significant insert performance improvements, nested document syntax, and native binaries for CentOS / RHEL.

Read the [release blog post][1.7-blog] for more details.

[1.7-blog]: http://rethinkdb.com/blog/1.7-release/


## New Features ##

* ReQL
  * Added `r.json` for parsing JSON strings server-side (#887)
  * Added syntax to `pluck` to access nested documents (#889)
  * `get_all` now takes a variable number of arguments (#915)
  * Added atomic set and get operations (#976)
    * `update`, `insert`, `delete` and `replace` now take an optional `return_vals` argument that returns the values that have been atomically modified
  * Renamed `getattr` to `get_field` and make it polymorphic on arrays (#993)
  * Drivers now use faster protobuf libraries when possible (#1027, #1026, #1025)
  * Drivers now use `r.json` to improve the performance of inserts (#1085)
  * Improved the behaviour of `pluck` (#1095)
    * A field with a non-pluckable value is considered absent
* Web UI
  * It is now possible to resolve `auth_key` conflicts via the web UI (#1028)
  * The web UI now uses relative paths (#1053)
* Server
  * Flushes to disk less often to improve performance without affecting durability (#520)
* CLI
  * Import and export JSON and CSV files (#193)
    * Four new subcommands: `rethinkdb import`, `rethinkdb export`, `rethinkdb dump` and `rethinkdb restore`
  * `rethinkdb admin` no longer requires the `--join` option (#1052)
    * It now connects to localhost:29015 by default
* Documentation
  * Documented durability settings correctly (#1008)
  * Improved instructions for migration (#1013)
  * Documented the allowed character names for tables (#1039)
* Packaging
  * RPMs for CentOS (#268)

## Fixed Bugs ##

* ReQL
  * Fixed the behaviour of `between` with `null` and secondary indexes (#1001)
* Web UI
  * Moved to using a patched Bootstrap, Handlebars.js 1.0.0 and LESS 1.4.0 (#954)
  * Fixed bug causing nested arrays to be shown as `{...}` in the Data Explorer (#1038)
  * Inline comments are now parsed correctly in the Data Explorer (#1060)
  * Properly remove event listeners from the dashboard view (#1044)
* Server
  * Fixed a crash caused by shutting down the server during secondary index creation (#1056)
  * Fixed a potential btree corruption bug (#986)
* Tests
  * ReQL tests no longer leave zombie processes (#1055)

---

# Release 1.6.1 (Fargo)

Released on 2013-06-19

## Critical Security Fixes ##

* Fixed a buffer overflow in the networking code
* Fixed a possible timing attack on the API key

## Other Changes ##

* In python, `r.table_list()` no longer throws an error (#1005)
* Fixed compilation failures with clang 3.2 (#1006)
* Fixed compilation failures with gcc 4.4 (#1011)
* Fixed problems with resolving a conflicted auth_key (#1024)

---

# Release 1.6.0 (Fargo)

Released on 2013-06-13

This release introduces basic access control, regular expression matching, new array operations, random sampling, better error handling, and many bug fixes.

Read the [release blog post][1.6-blog] for more details.

[1.6-blog]: http://rethinkdb.com/blog/1.6-release/

## New Features ##

* ReQL
  * Added access control with a single, common API key (#266)
  * Improved handshake to the client driver protocol (#978)
  * Added `sample` command for random sampling of sequences (#861, #182)
  * Secondary indexes can be queried with boolean values (#854)
  * Added `onFinished` callback to `each` in JavaScript to improve cursors (#443)
  * Added per-command durability settings (#890)
    * Changed `hard_durability=True` to `durability = 'soft' | 'hard'`
    * Added a `durability` option to `run`
  * Added `with_fields` command, and `pluck` no longer throws on missing attributes (#886)
  * Renamed `contains` to `has_fields` (#885)
    * `has_fields` runs on tables, objects, and arrays, and doesn't throw on missing attributes
    * `contains` now checks if an element is in an array
  * Added `default` command: replaces missing fields with a default value (#884)
  * Added new array operations (#868, #198, #341)
      * `prepend`: prepends an element to an array
      * `append`: appends an element to an array
      * `insert_at`: inserts an element at the specified index
      * `splice_at`: splices a list into another list at the specified index
      * `delete_at`: deletes the element at the specified index
      * `change_at`: changes the element at the specified index to the specified value
      * `+` operator: adds two arrays -- returns the ordered union
      * `*` operator: repeats an array `n` times
      * `difference`: removes all instances of specified elements from an array
      * `count`: returns the number of elements in an array
      * `indexes_of`: returns positions of elements that match the specified value in an array
      * `is_empty`: check if an array or table is empty
      * `set_insert`: adds an element to a set
      * `set_intersection`: finds the intersection of two sets
      * `set_union`: returns the union of two sets
      * `set_difference`: returns the difference of two sets
  * Added `match` command for regular expression matching (#867)
  * Added `keys` command that returns the fields of an object (#181)
* Web UI
  * Document fields are now sorted in alphabetical order in the table view (#832)
* CLI
  * Added `-v` flag as an alias for `--version` (#839)
  * Added `--io-threads` flag to allow limiting the amount of concurrent I/O operations (#928)
* Build system
  * Allow building the documentation with Python 3 (#826, #815)
  * Have `make` build the Ruby and Python drivers (#923)

## Fixed Bugs ##

* Server
  * Fixed a crash caused by resharding during secondary index creation (#852)
  * Fixed style problems: code hygiene (#805)
  * Server code cleanup (#920, #924)
  * Properly check permissions for writing the pid file (#916)
* ReQL
  * Use the correct function name in backtraces (#995)
  * Fixed callback issues in the JavaScript driver (#846)
  * In JavaScript, call the callback when the connection is closed (#758)
  * In JavaScript, GETATTR now checks the argument count (#992)
  * Tweaked the CoffeeScript source to work with older versions of coffee and node (#963)
  * Fixed a typo in the error handling of the JavaScript driver (#961)
  * Fixed performance regression for `pluck` and `without` (#947)
  * Make sure callbacks get cleared in the Javascript driver (#942)
  * Improved errors when `return` is omitted in Javascript (#914)
* Web UI
  * Correctly distinguish case sensitive table names (#822)
  * No longer display some results as `{ ... }` in the table view (#937)
  * Ensured tables are available before listing their indexes (#926)
  * Fixed the autocompletion for opening square brackets (#917)
* Tests
  * Modified the polyglot test framework to catch more possible errors (#724)
  * Polyglot tests can now run in debug mode (#614)
* Build
  * Look for `libprotobuf` in the correct path (#860, #853)
  * Always fetch and use the same version of Handlebars.js (#821, #819, #958)
  * Check for versions of LESS that are known to work (#956)
  * Removed bitrotted MOCK_CACHE_CHECK option (#804)

---

# Release 1.5.2 (Le Gras de Ouate)

Released on 2013-05-24

Bug fix update.

## Changes ##

* Fix #844: Compilation error when using `./configure --fetch protobuf` resolved
* Fix #840: Using `.run` in the data explorer gives a more helpful error message
* Fix #817: Fix a crash caused by adding a secondary index while under load
* Fix #831: Some invalid queries no longer cause a crash

---

# Release 1.5.1 (The Grad You Ate) #

Released on 2013-05-16

Bug fix update.

## Changes ###

* Fix a build error
* Fix #816: Table view in the data explorer is no longer broken

---

# Release 1.5.0 (The Graduate)

Released on 2013-05-16

This release introduces secondary indexes, stability and performance enhancements and many bug fixes.

Read the [release blog post][1.5-blog] for more details.

[1.5-blog]: http://rethinkdb.com/blog/1.5-release/

## New Features ##

* Server
  * #69: Bad JavaScript snippets can no longer hang the server
    * `r.js` now takes an optional `timeout` argument
  * #88: Added secondary and compound indexes
  * #165: Backtraces on OS X
  * #207: Added soft durability option
  * #311: Added daemon mode (via --daemon)
  * #423: Allow logging to a specific file (via --log-file)
  * #457: Significant performance improvement for batched inserts
  * #496: Links against libc++ on OS X
  * #672: Replaced environment checkpoints with shared pointers in the ReQL layer, reducing the memory footprint
  * #715: Adds support for noreply writes
* CLI
  * #566: In the admin CLI, `ls tables --long` now includes a `durability` column
* Web UI
  * #331: Improved data explorer indentation and brace pairing
  * #355: Checks for new versions of RethinkDB from the admin UI
  * #395: Auto-completes databases and tables names in the data explorer
  * #514: Rewrote and improved the documentation generator
  * #569: Added toggle for Data Explorer brace pairing
  * #572: Changed button styles in the Data Explorer
  * #707: Two step confirmation added to the delete database button
* ReQL
  * #469: Added an `info` command that provides information on any ReQL value (useful for retreiving the primary key)
  * #604: Allow iterating over result arrays as if they were cursors in the JavaScript client
  * #670: Added a soft durability option to `table_create`
* Testing
  * #480: Run [tests on Travis-CI](https://travis-ci.org/rethinkdb/rethinkdb)
  * #517: Improved the integration tests
  * #587: Validate API documentation examples
  * #669: Added color to `make test` output

## Fixed bugs ##

* Server
  * Fix #403: Serialization of `perfmon_result_t` is no longer 32-bit/64-bit dependent
  * Fix #505: Reduced memory consumption to avoid failure on read query "tcmalloc: allocation failed"
  * Fix #510: Make bind to `0.0.0.0` be equivalent to `all`
  * Fix #639: Fixed bug where 100% cpu utilization was reported on OSX
  * Fix #645: Fixed server crash when updating replicas / acks
  * Fix #665: Fixed queries across shards that would trigger `Assertion failed: [!br.point_replaces.empty()] in src/rdb_protocol/protocol.cc`
  * Fix #750: Resolved intermittent RPCSemilatticeTest.MetadataExchange failure
  * Fix #757: Fixed a crashing bug in the environment checkpointing code
  * Fix #796: Crash when running batched writes to a sharded table in debug mode
* Documentation
  * Fix #387: Documented the command line option --web-static-directory (custom location for web assets)
  * Fix #503: Updated outdated useOutdated documentation
  * Fix #563: Fixed documentation for `typeof()`
  * Fix #610: Documented optional arguments for `table_create`: `datacenter` and `cache_size`
  * Fix #631: Fixed missing parameters in documentation for `reduce`
  * Fix #651: Updated comments in protobuf file to reflect current spec
  * Fix #679: Fixed incorrect examples in protobuf spec
  * Fix #691: Fixed faulty example for `zip` in API documentation
* Web UI
  * Fix #529: Suggestion arrow no longer goes out of bounds
  * Fix #583: Improved Data Explorer suggestions for `table`
  * Fix #584: Improved Data Explorer suggestion style and content
  * Fix #623: Improved margins on modals
  * Fix #629: Updated buttons and button styles on the Tables View (and throughout the UI)
  * Fix #662: Command key no longer deletes text selection in Data Explorer
  * Fix #725: Improved documentation in the Data Explorer
  * Fix #790: False values in a document no longer show up as blank in the tree view
* ReQL
  * Fix #46: Added top-level `tableCreate`, `tableDrop` and `tableList` to drivers
  * Fix #125: Grouped reductions now return a stream, not an array
  * Fix #370: `map` after `dbList` and `tableList` no longer errors
  * Fix #421: Return values from mutation operations now include all attributes, even if zero (`inserted`,`deleted`,`skipped`,`replaced`,`unchanged`,`errors`)
  * Fix #525: Improved error for `groupBy`
  * Fix #527: Numbers that can be represented as integers are returned as native integers
  * Fix #603: Check the number of arguments correctly in the JavaScript client
  * Fix #632: Keys of objects are no longer magically converted to strings in the drivers
  * Fix #640: Clients handle system interrupts better
  * Fix #642: Ruby client no longer has a bad error message when queries are run on a closed connection
  * Fix #650: Python driver successfully sends `STOP` query when the connection is closed
  * Fix #663: Undefined values are handled correctly in JavaScript objects
  * Fix #678: `limit` no longer automatically converts streams to arrays
  * Fix #689: Python driver no longer hardcodes the default database
  * Fix #704: `typeOf` is no longer broken in JavaScript driver
  * Fix #730: Ruby driver no longer accepts spurious methods
  * Fix #733: `groupBy` can now handle both a `MAKE_OBJ` protobuf and a literal `R_OBJECT` datum protobuf
  * Fix #767: Server now detects `NaN` in all cases
  * Fix #777: Ruby driver no longer occasionally truncates messages
  * Fix #779: Server now rejects strings that contain a null byte
  * Fix #789: Style improvements in Ruby driver
  * Fix #799: Python driver handles `use_outdated` when specified as a global option argument
* Testing
  * Fix #653: `make test` now works on OS X
* Build
  * Fix #475: Added a workaround to avoid `make -j2` segfaulting when building on older versions of `make`
  * Fix #541: `make clean` is more aggressive
  * Fix #630: Removed cruft from the RethinkDB source tree
  * Fix #635: Building fails quickly with troublesome versions of `coffee-script`
  * Fix #694: Improved warnings when building with `clang` and `ccache`
  * Fix #717: Fixed `./configure --fetch-protobuf`
* Migration
  * Fix #782: Improved speed in migration script

## Other ##

  * #508: Removed the JavaScript mock server
  * [All closed issues between 1.4.5 and 1.5.0](https://github.com/rethinkdb/rethinkdb/issues?milestone=8&page=1&state=closed)

---

# Release 1.4.5 (Some Like It Hot) #

Released on 2013-05-03.

## Changes ##

Bug fix update:

* Increase the TCP accept backlog (#369).

---

# Release 1.4.4 (Some Like It Hot) #

Released on 2013-04-19.

## Changes ##

Bug fix update:

* Improved the documentation
  * Made the output of `rethinkdb help` and `--help` consistent (#643)
  * Clarify details about the client protocol (#649)
* Cap the size of data returned from rgets not just the number of values (#597)
  * The limit changed from 4000 rows to 1MiB
* Partial bug fix: Rethinkdb server crashing (#621)
* Fixed bug: Can't insert objects that use 'self' as a key with Python driver (#619)
* Fixed bug: [Web UI] Statistics graphs are not plotted correctly in background tabs (#373)
* Fixed bug: [Web UI] Large JSON Causes Data Explorer to Hang (#536)
* Fixed bug: Import command doesn't import last row if doesn't have end-of-line (#637)
* Fixed bug: [Web UI] Cubism doesn't unbind some listeners (#622)
* Fixed crash: Made global optargs actually propagate to the shards properly (#683)

---

# Release 1.4.3 (Some Like It Hot) #

Released on 2013-04-10.

## Changes ##

Bug fix update:

* Improve the networking code in the Python driver
* Fix a crash triggered by a type error when using concatMap (#568)
* Fix a crash when running `rethinkdb proxy --help` (#565)
* Fix a bug in the Python driver that caused it to occasionally return `None` (#564)

---

# Release 1.4.2 (Some Like It Hot) #

Released on 2013-03-30.

## Changes ##

Bug fix update:

* Replace `~` with `About` in the web UI (#485)
* Add framing documentation to the protobuf spec (#500)
* Fix crashes triggered by .orderBy().skip() and .reduce(r.js()) (#522, #545)
* Replace MB with GB in an error message (#526)
* Remove some semicolons from the protobuf spec (#530)
* Fix the `rethinkdb import` command (#535)
* Improve handling of very large queries in the data explorer (#536)
* Fix variable shadowing in the javascript driver (#546)

---

# Release 1.4.1 (Some Like It Hot) #

Released on 2013-03-22.

## Changes ##

Bug fix update:

* Python driver fix for TCP streams (#495)
* Web UI fix that reduces the number of AJAX requests (#481)
* JS driver: added useOutdated to r.table() (#502)
* RDB protocol performance fix in release mode.
* Performance fix when using filter with object shortcut syntax.
* Do not abort when the `runuser` or `rungroup` options are present (#512)

---

# Release 1.4.0 (Some Like It Hot)

Relased on 2013-03-18.

## Changes ##

* Improved ReQL wire protocol and client drivers
* New build system
* Data explorer query history

---

# Release 1.3.2 (Metropolis) #

Released on 2013-01-15.

## Changes ##

* Fixed security bug in http server.

---

# Release 1.3.1 (Metropolis) #

Released on 2012-12-20.

## Changes ##

* Fixed OS X crash on ReQL exceptions.

---

# Release 1.3.0 (Metropolis)

Released on 2012-12-20.

## Changes ##

* Native OS X support.
* 32-bit support.
* Support for legacy systems (e.g. Ubuntu 10.04)

---

# Release 1.2.8 (Rashomon) #

Released on 2012-12-15.

## Changes ##

* Updating data explorer suggestions to account for recent `r.row`
  changes.

---

# Release 1.2.7 (Rashomon) #

Released on 2012-12-14.

## Changes ##

* Lots and lots of bug fixes

---

# Release 1.2.6 (Rashomon) #

Released on 2012-11-13.

## Changes ##

* Fixed the version string
* Fixed 'crashing after crashed'
* Fixed json docs

---

# Release 1.2.5 (Rashomon) #

Released on 2012-11-11.

## Changes ##

* Checking for a null ifaattrs

---

# Release 1.2.4 (Rashomon) #

Released on 2012-11-11.

## Changes ##

* Local interface lookup is now more robust.

---

# Release 1.2.3 (Rashomon) #

Released on 2012-11-09.

## Changes ##

* Fixes a bug in the query engine that causes large range queries to
  return incorrect results.

---

# Release 1.2.0 (Rashomon)

Released on 2012-11-09.

## Changes ##

This is the first release of the product. It includes:

* JSON data model and immediate consistency support
* Distributed joins, subqueries, aggregation, atomic updates
* Hadoop-style map/reduce
* Friendly web and command-line administration tools
* Takes care of machine failures and network interrupts
* Multi-datacenter replication and failover
* Sharding and replication to multiple nodes
* Queries are automatically parallelized and distributed
* Lock-free operation via MVCC concurrency

## Limitations ##

There are a number of technical limitations that aren't baked into the
architecture, but were not resolved in this release due to time
pressure. They will be resolved in subsequent releases.

* Write requests have minimal batching in memory and are flushed to
  disk on every write. This significantly limits write performance,
  expecially on rotational disks.
* Range commands currently don't use an index.
* The clustering system has a bottleneck in cluster metadata
  propagation. Cluster management slows down significantly when more
  than sixteen shards are used.

