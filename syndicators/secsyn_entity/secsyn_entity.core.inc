<?php

function node_secsyn_entity_presave(&$object) {
  $info = entity_get_info($object->entity_type);
  if ($object->entity_type == 'node') {
    if (!empty($object->tnid)) {
      if ($object->tnid == $object->{$info['entity keys']['uuid']}) {
        $object->secsyn_state['selfref_translation'] = TRUE;
        $object->tnid = 0;
      }
    }
  }
}

function node_secsyn_entity_postsave(&$object) {
  if ($object->entity_type) {
    if (!empty($object->secsyn_state['selfref_translation'])) {
      $object->tnid = $object->nid;
      unset($object->secsyn_state);
      entity_uuid_save($object->entity_type, $object);
    }
  }
}

function node_secsyn_entity_children($entity) {
  $children = array();
  if ($entity->entity_type == 'node') {
    if (module_exists('comment')) {
      $comments = db_query('SELECT uuid FROM {comment} WHERE nid = :nid', array(
        ':nid' => $entity->nid,
      ));
      foreach ($comments as $comment) {
        $children[] = array(
          'profile' => 'SecureSyndicationEntityProfile',
          'object' => reset(entity_uuid_load('comment', array($comment->uuid))),
        );
      }
    }
  }
  return $children;
}

function node_secsyn_entity_dependency($entity) {
  $dependencies = array();
  if ($entity->entity_type == 'node') {
    if (!empty($entity->tnid)) {
      if ($entity->tnid != $entity->nid) {
        $dependencies[] = array(
          'profile' => 'SecureSyndicationEntityProfile',
          'object' => reset(entity_uuid_load('node', array($entity->tnid))),
        );
      }
    }
    // we don't want to syndicate profiles for user 0 or 1, that would be crazy
    if ($entity->uid > 1) {
      /*$dependencies[] = array(
        'profile' => 'SecureSyndicationBaseUserProfile',
        'object' => reset(entity_uuid_load('user', array($entity->uid))),
      );*/
    }
  }
  return $dependencies;
}

function comment_secsyn_entity_dependency($entity) {
  $dependencies = array();
  if ($entity->entity_type == 'comment') {
    if ($entity->uuid > 1) {
      /*$dependencies[] = array(
        'profile' => 'SecureSyndicationBaseUserProfile',
        'object' => reset(entity_uuid_load('user', array($entity->uid))),
      );*/
    }
    if (!empty($entity->pid)) {
      $parent = comment_load($entity->pid);
      $dependencies[] = array(
        'profile' => 'SecureSyndicationEntityProfile',
        'object' => reset(entity_uuid_load('comment', array($parent->uuid))),
      );
    }
  }
  return $dependencies;
}

function taxonomy_secsyn_entity_dependency($entity) {
  $dependencies = array();
  if ($entity->entity_type == 'taxonomy_term') {
    $dependencies[] = array(
      'profile' => 'SecureSyndicationVocabularyProfile',
      'object' => taxonomy_vocabulary_machine_name_load($entity->vocabulary_machine_name),
    );
    $parents = taxonomy_get_parents($entity->tid);
    foreach ($parents as $parent) {
      $dependencies[] = array(
        'profile' => 'SecureSyndicationEntityProfile',
        'object' => $parent,
      );
    }
  }
  return $dependencies;
}

function taxonomy_secsyn_entity_presave(&$object) {
  if ($object->entity_type == 'taxonomy_term') {
    if (!empty($object->parents)) {
      $object->secsyn_state['parents'] = $object->parents;
      unset($object->parents);
    }
  }
}

function taxonomy_secsyn_entity_postsave(&$object) {
  if ($object->entity_type == 'taxonomy_term') {
    if (!empty($object->secsyn_state['parents'])) {
      // rebuild the hierarchy, to match the syndicated version
      db_delete('taxonomy_term_hierarchy')
          ->condition('tid', $object->tid)
          ->execute();
      foreach ($object->secsyn_state['parents'] as $uuid) {
        db_insert('taxonomy_term_hierarchy')
            ->fields(array(
              'tid' => $object->tid,
              'parent' => entity_get_id_by_uuid('taxonomy_term', $uuid),
            ))
            ->execute();
      }
    }
  }
}

function taxonomy_secsyn_field_dependency($field, $instance, $entity) {
  $dependencies = array();
  if ($field['type'] == 'taxonomy_term_reference') {
    foreach ($entity->{$instance['field_name']} as $lang => $list) {
      foreach ($list as $uuid) {
        $term = entity_uuid_load('taxonomy_term', array($uuid['tid']));
        $term = reset($term);
        $dependencies[] = array(
          'profile' => 'SecureSyndicationEntityProfile',
          'object' => $term,
        );
      }
    }
  }
  return $dependencies;
}

function taxonomy_secsyn_entity_package_alter(&$package, $entity) {
  if ($package->entity_type == 'taxonomy_term') {
    $parents = taxonomy_get_parents($entity->tid);
    $package->parents = array();
    foreach ($parents as $parent) {
      $package->parents[] = $parent->uuid;
    }
  }
}

function user_secsyn_entity_package_alter(&$package, $entity) {
  if ($package->entity_type == 'user') {
    unset($entity->pass);
    unset($entity->status);
  }
}

function user_secsyn_entity_presave(&$object) {
  if ($object->entity_type == 'user') {
    $profile = SecureSyndicationProfile::getProfile('SecureSyndicationBaseUserProfile');
    $existing = $profile->lookup($profile->uuid($object));
    if (empty($existing)) {
      $object->status = 0;
      $object->pass = NULL;
    }
  }
}