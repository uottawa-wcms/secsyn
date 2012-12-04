<?php

abstract class SecureSyndicationProfile {

  private static $profiles = array();

  /**
   *
   * @return SyndicationProfile
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

  public function dependencies($object) {
    return array();
  }

  public function children($object) {
    return array();
  }

  public abstract function package($object);

  public abstract function update($uuid, $fields);

  public abstract function lookup($uuid);

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

  public abstract function uuid($object);

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