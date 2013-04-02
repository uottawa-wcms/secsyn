<?php

/**
 * Parent class for all profiles - defines methods that must be implemented
 * and provides some very basic functionality.
 *
 * Generally the methods implemented in this object must be stateless - they
 * should not rely on any instance variables being present (unless they are
 * only touched through the constructor).
 *
 * Before creating a profile, you should consider two things in detail:
 *
 * 1. The object you will be syndicating.
 *
 * Syndication happens on objects (this does not literally have to be an
 * instance of a class, merely on a chunk of data). You should consider what
 * form the data will take (class instance, associative array, etc) and how
 * the following will happen:
 *
 * - stripping the object down for transport (removing values)
 * - updating certain values but not others
 * - finding an instance of the object
 * - saving an updated or new copy of the object
 *
 * 2. A unique identifier for the object
 *
 * Each object requires a UUID for it. This UUID must be:
 *
 * - the same for the same object on different Drupal installations (ie not a
 * serial ID).
 * - a string, integer or other non-complex data type
 * - able to be used as a lookup for the object
 * - completely identify one and only one object
 *
 * Some good examples of UUIDs are:
 *
 * - The UUID provided by the uuid project (for entities)
 * - The user's email address
 * - The taxonomy vocabulary machine name
 *
 *
 * Here is a rough outline of the syndication process. LOCAL is the server
 * sending the object, REMOTE is the server receiving it.
 *
 * 1. LOCAL is passed the UUID of the object to start.
 * 2. LOCAL loads the object using lookup().
 * 3. LOCAL adds the dependencies() to the queue first
 * 4. LOCAL adds the object to the queue
 * 5. LOCAL adds the children() to the queue
 * 6. LOCAL retrieves the hash() of the object using no lookup fields
 * 7. LOCAL retrieves the version() of the profile
 * 8. LOCAL passes the hash, profile classname, version, lookup fields and uuid over to REMOTE
 * 9. REMOTE retrieves the profile and makes sure it exists
 * 10. REMOTE retrieves the version() of the profile and ensures it matches.
 * 11. REMOTE uses lookup() to find the object
 * 12. If the object does not exist, REMOTE requests that it be sent
 * 13. If the object does exist, REMOTE retrieves the hash() using the lookup fields passed
 * 14. If the hashes don't match, REMOTE requests that the object be sent
 * 15. LOCAL calls package() for the object
 * 16. LOCAL sends the profile classname, version, uuid and packaged data to REMOTE
 * 17. REMOTE repeats steps 9 and 10 for the profile
 * 18. REMOTE calls update() with the uuid and packaged data
 * 19. REMOTE responds that the object has been updated
 */
abstract class SecureSyndicationProfile {

  /**
   * Multiton pattern
   * @var array
   */
  private static $profiles = array();

  /**
   * Multiton profile retrieval.
   *
   * Ensures there is only ever one instance of each profile.
   *
   * @todo Is multiton appropriate here? Should we remove the pattern (and make
   * this a factory) so that profiles could use instance variables?
   *
   * @param string $profile_name The name of a class extending this one.
   *
   * @return SecureSyndicationProfile A subclass of SecureSyndicationProfile
   */
  public static function getProfile($profile_name) {
    if (!isset(self::$profiles[$profile_name])) {
      $profile = NULL;
      // make sure the class exists
      if (drupal_autoload_class($profile_name)) {
        $profile = new $profile_name;
        // we only want subclasses of SyndicationProfile
        if (!is_subclass_of($profile, 'SecureSyndicationProfile')) {
          $profile = NULL;
        }
      }
      self::$profiles[$profile_name] = $profile;
    }
    return self::$profiles[$profile_name];
  }

  /**
   * Returns a list of dependencies for the object.
   *
   * Dependencies are associative arrays with two keys:
   * - profile: The classname of the profile to use
   * - object: The object to be passed into the profile
   *
   * A dependency is an object that should be queued for syndication prior to
   * this object being syndicated. Generally it represents a parent in a
   * parent-child relationship that is mandatory.
   *
   * @param mixed $object The object currently being processed, as returned by
   *   lookup().
   * @return array An array of dependencies.
   */
  public function dependencies($object) {
    return array();
  }

  /**
   * Returns a list of child objects.
   *
   * Child objects are associative arrays in same format as dependencies:
   * - profile: The classname of the profile to use
   * - object: The object to be passed into the profile
   *
   * A child object though is one that should be queued for syndication after
   * this one is syndicated. It generally represents child objects for a
   * composite object.
   *
   * @param mixed $object The object currently being processed, as returned by
   *   lookup().
   * @return array An array of child objects
   */
  public function children($object) {
    return array();
  }

  /**
   * Packages this object for transport.
   *
   * This should strip out all the unnecessary fields from the object and
   * create a compacted array. This array will then be passed as the $fields
   * parameter in the update() method.
   *
   * @param mixed $object The object currently being processed, as returned by
   *   lookup().
   * @return array An array of data that will be processed by update().
   *
   */
  public abstract function package($object);

  /**
   * Updates the local copy of the object using the fields provided.
   *
   * $fields may not be fields - it is simply an array of information about
   * the object from the remote server as returned by package().
   *
   * @param string $uuid The unique identifier for this object, as returned by
   *   uuid().
   * @param array $fields The values to update on the object.
   */
  public abstract function update($uuid, $fields);

  /**
   * Returns an object based on a UUID.
   *
   * This function should be the mirror of uuid() - that is, calling
   * lookup(uuid(X)) should equal X and calling uuid(lookup(Y)) should equal Y.
   *
   * @param string $uuid The unique identifer fo0r this object.
   */
  public abstract function lookup($uuid);

  /**
   * Returns the UUID for an object.
   *
   * This function should be the mirror of lookup().
   *
   * @param mixed $object An object appropriate for this profile.
   * @return string The appropriate UUID.
   */
  public abstract function uuid($object);

  /**
   * Generates a hash value of the object.
   *
   * This hash is used to compare the local and remote version of an object,
   * to determine if an update is needed.
   *
   * The $lookup_fields variable can be used to specify which fields should be
   * used. For example, if we are only syndicating the node title, then
   * array('title') would be passed - we then know to only hash the title
   * value.
   *
   * By default, the object is packaged first before being hashed.
   *
   * @param mixed $object An object appropriate for this profile.
   * @param array|NULL $lookup_fields A list of object properties or array keys
   *   to compare.
   * @return array The sha-384 hash of the serialized version of the packaged
   *   object and the lookup fields that were used.
   */
  public function hash($object, $lookup_fields = NULL) {
    $object = $this->package($object);
    if (is_array($object)) {
      $object = (object) $object;
    }
    if (!empty($lookup_fields)) {
      $props = array_keys(get_object_vars($object));
      foreach ($props as $property) {
        if (!in_array($property, $lookup_fields)) {
          unset($object->{$property});
        }
      }
    }
    $hash = hash('sha384', serialize($object));
    return array(
      'hash' => $hash,
      'lookup_fields' => get_object_vars($object),
    );
  }

  /**
   * The version of the profile.
   *
   * If you make changes to the profile that are NOT backwards compatible,
   * you should implement this function to return a higher version number.
   *
   * Part of the validation process ensures that the version numbers match
   * between local and remote servers.
   *
   * @return int The current version number.
   */
  public function version() {
    return 1;
  }

  /**
   * These methods are included in support of secsyn_filters.
   */
  public function filters() {
    return array();
  }

  public function filteredList($filter_params) {
    return array();
  }

  public function countList($filter_params) {
    return count($this->filteredList($filter_params));
  }

  public function queueAll($remote_server, $filter_params, $urgent = FALSE) {
    return TRUE;
  }

}