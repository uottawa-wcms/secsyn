<?php

class SecureSyndicationBundleProfile extends SecureSyndicationProfile {

  public function children($object) {
    $children = array();
    $instances = field_info_instances($object->entity_type, $object->bundle_name);
    foreach ($instances as $instance) {
      // TODO: Ignore certain fields
      $children[] = array(
        'profile' => 'SecureSyndicationFieldInstanceProfile',
        'object' => $instance,
      );
    }
    return $children;
  }

  public function package($object) {
    return (array) $object;
  }

  public function update($uuid, $fields) {
    $pieces = explode('/', $uuid);
    $fields = (object) $fields;
    $fields->entity_type = $pieces[0];
    $fields->bundle_key = $pieces[1];
    return bundle_api_save_bundle($fields);
  }

  public function lookup($uuid) {
    $pieces = explode('/', $uuid);
    return bundle_api_get_bundle($pieces[0], $pieces[1]);
  }

  public function uuid($object) {
    if (empty($object->entity_type) || empty($object->bundle_name)) {
      return FALSE;
    }
    if (!bundle_api_support($object->entity_type)) {
      return FALSE;
    }
    return $object->entity_type . '/' . $object->bundle_name;
  }

}
