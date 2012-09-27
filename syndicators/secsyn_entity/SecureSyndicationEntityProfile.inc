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
    $this->invokeHook('secsyn_entity_package_alter', $package);
    return $package;
  }

  private function invokeHook($hookname, &$object) {
    $modules = module_implements($hookname);
    foreach ($modules as $module) {
      $method = $module . '_' . $hookname;
      if (function_exists($method)) {
        $method($object);
      }
    }
  }

  public function update($uuid, $fields) {
    $uuid = $this->splitUUID($uuid);
    $info = entity_get_info($uuid['entity_type']);
    $fields->entity_type = $uuid['entity_type'];
    $uuid_field = $info['entity keys']['uuid'];
    $fields->$uuid_field = $uuid['uuid'];
    $object->secsyn_state = array();
    $this->invokeHook('secsyn_entity_presave', $fields);
    $state = $object->secsyn_state;
    unset($object->secsyn_state);
    entity_uuid_save($uuid['entity_type'], $fields);
    $object->secsyn_state = $state;
    $this->invokeHook('secsyn_entity_postsave', $fields);
    return TRUE;
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

  public function entityTypeFilter($values) {
    $options = array();
    $e_info = entity_get_info();
    foreach ($e_info as $e_key => $entity) {
      if (!empty($entity['entity keys']['uuid'])) {
        if (count($entity['bundles']) > 1) {
          $options[$e_key] = $entity['label'] . ' - [all bundles]';
          foreach ($entity['bundles'] as $bundle_key => $bundle) {
            $options[$e_key . '/' . $bundle_key] = $entity['label'] . ' - ' . $bundle['label'];
          }
        } else {
          $options[$e_key] = $entity['label'];
        }
      }
    }
    return array(
      'entity_type' => array(
        '#title' => t('Entity Type / Bundle'),
        '#type' => 'select',
        '#default_value' => $values['entity_type'],
        '#empty_value' => '',
        '#empty_option' => 'Select one...',
        '#options' => $options,
      )
    );
  }

  public function entityTypeValidate($element_name, $values) {
    if (empty($values['entity_type'])) {
      form_set_error($element_name . '][entity_type', t('You must pick an entity type or bundle'));
    }
    return TRUE;
  }

  public function entityTypeFilterValues($form_state) {
    return array(
      'entity_type' => $form_state['entity_type'],
    );
  }

  public function filteredList($filter_params) {
    $list = array();
    $queries = $this->getQueries($filter_params);
    foreach ($queries as $efq) {
      $results = $efq->execute();
      foreach ($results as $type => $entities) {
        foreach ($entities as $entity_id => $entity) {
          $object = reset(entity_load($type, $entity));
          $uri = entity_uri($type, $object);
          $standard = array(
            'label' => entity_label($type, $object),
            'link' => $uri['path'],
            'uuid' => $this->uuid($object),
            'preview' => entity_view($type, array($object), 'secsyn_preview'),
          );
          $list[] = $standard;
        }
      }
    }
    return $list;
  }

  public function queueAll($remote_server, $filter_params, $urgent = FALSE) {
    $queries = $this->getQueries($filter_params);
    $engine = new SecureSyndicationEngine();
    foreach ($queries as $efq) {
      $results = $efq->execute();
      foreach ($results as $type => $entities) {
        foreach ($entities as $entity) {
          $results = entity_load($type, $entity);
          $engine->queue($remote_server, 'SecureSyndicationEntityProfile', reset($results));
        }
      }
    }
    return TRUE;
  }

  private function getQueries($filter_params) {
    $queries = array();
    foreach ($filter_params['filters'] as $filter) {
      if ($filter['filter_key'] == 'entity_type') {
        $type = $filter['filter_options']['entity_type'];
        if (!empty($type)) {
          if (strpos($type, '/') !== FALSE) {
            $pieces = explode('/', $type, 2);
          } else {
            $pieces = array($type, '');
          }
          if (!isset($entity_types_allowed[$pieces[0]])) {
            $entity_types_allowed[$pieces[0]] = array();
          }
          $entity_types_allowed[$pieces[0]][] = $pieces[1];
        }
      }
    }
    foreach ($entity_types_allowed as $entity_type => $bundles) {
      $efq = new EntityFieldQuery();
      $efq->entityCondition('entity_type', $entity_type);
      if (count($bundles)) {
        $efq->entityCondition('bundle', $bundles, 'IN');
      }
      // TODO: ADD OTHER CONDITIONS HERE
      $queries[] = $efq;
    }
    return $queries;
  }

  public function filters() {
    $filters = parent::filters();
    $filters += array(
      'entity_type' => array(
        'title' => 'Entity Type / Bundle',
        'filter_form' => 'entityTypeFilter',
        'value_callback' => 'entityTypeFilterValues',
        'validate_callback' => 'entityTypeValidate',
        'multiple' => TRUE,
        'defaults' => array(
          'entity_type' => '',
        ),
      ),
      /*
      'field' => array(
        'title' => 'Field Value',
        'filter_form' => 'fieldFilter',
        'value_callback' => 'fieldFilterValues',
        'validate_callback' => 'fieldFilterValidate',
        'multiple' => TRUE,
        'defaults' => array(

        ),
      ),
       *
       */
    );
    return $filters;

  }

}