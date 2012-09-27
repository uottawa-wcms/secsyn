<?php

class SecureSyndicationFieldInstanceProfile extends SecureSyndicationProfile {

  public function dependencies($object) {
    $deps = array();
    $deps[] = array(
      'profile' => 'SecureSyndicationFieldProfile',
      'object' => field_info_field($object['field_name']),
    );
    return $deps;
  }

  public function package($object) {
    $package = (array) $object;
    unset($package['id']);
    unset($package['field_id']);
    return $package;
  }

  public function update($uuid, $fields) {
    $existing_object = $this->lookup($uuid);
    try {
      if (empty($existing_object)) {
        field_create_instance($fields);
      } else {
        field_update_instance($fields);
      }
      return TRUE;
    } catch (FieldException $e) {
      watchdog_exception('secsyn_bundle', $e, 'Error while trying to syndicate a field instance definition');
      return FALSE;
    }
  }

  public function lookup($uuid) {
    $pieces = explode('/', $uuid);
    return field_info_instance($pieces[0], $pieces[2], $pieces[1]);
  }

  public function uuid($object) {
    return $object['entity_type'] . '/' . $object['bundle'] . '/' . $object['field_name'];
  }

}
