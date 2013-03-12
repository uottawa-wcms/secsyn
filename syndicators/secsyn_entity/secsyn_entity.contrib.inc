<?php

function entityreference_secsyn_field_dependency($field, $instance, $entity) {
  $dependencies = array();
  if ($field['type'] == 'entityreference') {
    $entity_type = $field['settings']['target_type'];
    foreach ($entity->{$instance['field_name']} as $lang => $list) {
      foreach ($list as $uuid) {
        $ref = entity_uuid_load($entity_type, array($uuid['target_id']));
        $ref = reset($term);
        $dependencies[] = array(
          'profile' => 'SecureSyndicationEntityProfile',
          'object' => $ref,
        );
      }
    }
  }
  return $dependencies;
}
