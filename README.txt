Secure syndication is designed to support a multi-tiered environment for
running Drupal. Enterprise environments often include:

* production servers
* QA / staging servers
* development servers
* authoring servers

Trying to keep content the same between all these different servers can be
challenging. We've designed secure syndication to solve these challenges.

# Syndication Engine

The syndication engine drives the process behind syndication. There are three
queues of item that are processed:

Wait Queue:        Items here have been tagged as needing to be syndicated but
                   nothing has been done with them yet.
Validation Queue:  Items here have had their dependencies processed and
                   expanded. All required objects for the syndication process
                   should be in this queue
Syndication Queue: Items here have been checked on the remote server to ensure
                   that they do not exist and are identical.

WAIT QUEUE PROCESS:
1. An item is queued in the wait queue
2. The item is checked for dependencies.
   * If dependencies exist, they are added to the validation queue using steps
     2-4 themselves. Recursion is prevented using a unique queue.
3. The item is added to the validation queue
4. The item is checked for child items
   * If child items exist, they are added to the validation queue using steps
     2-4 themselves.

VALIDATION QUEUE PROCESS:
1. An item is queued in the validation queue.
2. The item has it's UUID looked up using it's profile.
3. The item has it's hash generated for it's current state using it's profile.
4. The item's UUID, hash and profile version are sent to the remote server (this
   happens in large batches).
5. The remote server ensures it's profile version is the same.
6. The remote server checks if the object exists
   * If it doesn't, the object is requested
7. If it does exist, the remote server checks if the hashes are the same
   * If they are not, the object is requested
8. The remote server sends the list of requested objects back to the origin
   server
9. The origin server queues all the requested objects into the syndication
   queue.

SYNDICATION QUEUE PROCESS:
1. An item is queued in the syndication queue.
2. The item has it's UUID looked up.
3. The item is packaged (strips all non-essential fields) and serialized.
4. The item is sent to the remote server (again, in batches)
5. The remote server unserializes the item and updates it's local copy.


# Concept: Syndication Profiles

A syndication profile is a class that extends SecureSyndicationProfile. It's
goal is to provide a specific implementation for the various methods required
for a type of object. For example, to syndicate entities, there is a profile
just for entities that relies on entity api to provide the required
functionality.

Some data that the syndication profile provides:

* dependencies(): objects that should be transferred before this one
* children(): objects that should be transferred after this one
* package(): creates a clean, minimal object that contains only the required
  information for the transfer
* update(): creates or updates the object on request
* lookup(): finds the object matching a unique identifier
* hash(): provides a unique string representation of the object's current state
* uuid(): provides the unique identifier for the object
* filters(): returns an array of filter fields to create a filter
* filteredList(): returns an array of objects that match a set of filters
* countList(): returns the number of items that match a set of filters
* queueAll(): queues all the items that match a set of filters


# Extensions

Numerous extensions have been written that provide extra value to the
syndication process. Currently 4 extensions exist:

* Syndication Filters: Allows the site admin to build a filter set using
  conditions returned by the filters() method of a profile.
* Auto Syndication: Queues all items in a filter to be syndicated to a given
  server every interval. Controlled by cron.
* Syndication Push: Allows the site admin to manually push items in a filter
  to a remote server.
* Syndication Pull: Allows the remote site admin to pull items from a filter on
  this server.