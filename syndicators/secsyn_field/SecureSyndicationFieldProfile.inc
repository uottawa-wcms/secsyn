<?php

class SecureSyndicationFieldProfile extends SecureSyndicationProfile {

  public function dependencies($object) {
    $dependencies = array();
    $dependencies += module_invoke_all('secsyn_field_dependencies', $object);
    return $dependencies;
  }

  public function package($object) {
    $package = $object;
    unset($package['id']);
    unset($package['bundles']);
    return $package;
  }

  public function update($uuid, $fields) {
    try {
      $existing_object = $this->lookup($uuid);
      if (empty($existing_object)) {
        field_create_field($fields);
      } else {
        field_update_field($fields);
      }
      return TRUE;
    } catch (FieldException $e) {
      watchdog(
          'secsyn_field',
          'Could not update @field',
          array('@field' => $uuid),
          WATCHDOG_WARNING
      );
      return FALSE;
    }
  }

  public function lookup($uuid) {
    return field_info_field($uuid);
  }

  public function uuid($object) {
    return $object['field_name'];
  }

}
