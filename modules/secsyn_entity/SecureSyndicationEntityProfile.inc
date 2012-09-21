<?php

class SecureSyndicationEntityProfile extends SecureSyndicationProfile {

  public function dependencies($object) {
    $dependencies = module_invoke_all('secsyn_entity_dependency', $object);
    $info = entity_get_info($object->entity_type);
    $bundle = NULL;
    if (isset($info['entity keys']['bundle'])) {
      $bundle = $object->{$info['entity keys']['bundle']};
    }
    $fields = field_info_instances($object->entity_type, $bundle);
    foreach ($fields as $field_instance) {
      $field = field_info_field($field_instance['field_name']);
      $dependencies = array_merge($dependencies,
          module_invoke_all('secsyn_field_dependency', $field, $field_instance, $object));
    }
    return $dependencies;
  }

  public function package($object) {
    $package = clone $object;
    $info = entity_get_info($object->entity_type);
    $bundle = NULL;
    if (isset($info['entity keys']['bundle'])) {
      $bundle = $info['entity keys']['bundle'];
    }
    $bundle = $info['entity keys']['bundle'];
    $ignore = array();
    $ignore[$info['entity keys']['id']] = $info['entity keys']['id'];
    if (isset($info['entity keys']['revision'])) {
      $ignore[$info['entity keys']['revision']] = $info['entity keys']['revision'];
    }
    if (isset($info['entity keys']['revision uuid'])) {
      $ignore[$info['entity keys']['revision uuid']] = $info['entity keys']['revision uuid'];
    }
    $invocation = module_invoke_all('secsyn_entity_ignore_fields', $object);
    $ignore += $invocation;
    foreach ($ignore as $field_name) {
      if (property_exists($package, $field_name)) {
        unset($package->$field_name);
      }
    }
    return $package;
  }

  public function update($uuid, $fields) {
    $uuid = $this->splitUUID($uuid);
    $info = entity_get_info($uuid['entity_type']);
    if (!empty($info['entity keys']['revision'])) {
      $fields->revision = 1;
    }
    $fields->entity_type = $uuid['entity_type'];
    $uuid_field = $info['entity keys']['uuid'];
    $fields->$uuid_field = $uuid['uuid'];
    // detect if this is a node with a tnid equal to itself
    $primary_translation = FALSE;
    if ($fields->entity_type == 'node') {
      if (!empty($fields->tnid)) {
        if ($fields->tnid == $fields->$uuid_field) {
          $primary_translation = TRUE;
          $fields->tnid = 0;
        }
      }
    }
    entity_uuid_save($uuid['entity_type'], $fields);
    // the tnid was not saved (because it could not be looked up properly)
    if ($primary_translation) {
      // restore the UUID for tnid, because the save will overwrite it.
      $fields->tnid = $fields->nid;
      // resaving the entity will fix this problem.
      entity_uuid_save($uuid['entity_type'], $fields);
    }
  }
  
  public function lookup($uuid) {
    $uuid = $this->splitUUID($uuid);
    return reset(entity_uuid_load($uuid['entity_type'], array($uuid['uuid'])));
  }

  public function uuid($object) {
    // we need the UUID to contain the entity type, so we can look it up on the
    // other side.
    $info = entity_get_info($object->entity_type);
    if (!isset($info['entity keys']['uuid'])) {
      return FALSE;
    }
    $uuid_property = $info['entity keys']['uuid'];
    return $object->entity_type . '/' . $object->$uuid_property;
  }

  private function splitUUID($uuid) {
    $pieces = explode('/', $uuid, 2);
    return array(
      'entity_type' => $pieces[0],
      'uuid' => $pieces[1],
    );
  }

  public function filters() {
    $entities = array_keys(entity_get_info());
    $entity_checkboxes = array();
    foreach ($entities as $key) {
      $entity_checkboxes[$key] = $key;
    }
    return array(
      'by_entity_type' => array(
        '#title' => t('Filter: Entity Type'),
        'selection_type' => array(
          '#type' => 'radio',
          '#title' => t('Mode'),
          '#options' => array(
            'inclusion' => 'Only syndicate selected entities',
            'exclusive' => 'Syndicate all but the selected entities',
          ),
          '#default_value' => 'exclusive',
        ),
        'entity_type' => array(
          '#type' => 'checkboxes',
          '#options' => $entity_checkboxes,
          '#title' => t('Entity Type'),
        ),
      ),
    );
  }

  public function queueAll($filters) {

  }

}