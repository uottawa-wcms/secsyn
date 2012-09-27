<?php

function secsyn_filters_filter_list() {
  $rows = array();
  $headers = array(
    'title' => t('Title'),
    'count' => t('Objects'),
    'ops' => t('Operations'),
  );
  $filters = db_query('SELECT * FROM {secsyn_filters_profile} ORDER BY title');
  foreach ($filters as $filter) {
    $profile = secsyn_filters_profile_load($filter->machine_name);
    $ops = array(
      //l('view', 'admin/content/syndication/filters/' . $filter->machine_name),
      l('edit', 'admin/content/syndication/filters/' . $filter->machine_name . '/edit'),
      l('delete', 'admin/content/syndication/filters/' . $filter->machine_name . '/delete'),
    );
    $rows[] = array(
      'title' => $filter->title,
      'count' => secsyn_filters_count_content($profile),
      'ops' => implode(' | ', $ops),
    );
  }
  return theme('table', array(
    'header' => $headers,
    'rows' => $rows,
  ));
}

function secsyn_filters_filter_form($form, &$form_state, $profile = NULL) {
  module_load_include('inc', 'secsyn', 'secsyn.functions');
  if (empty($profile)) {
    $profile = (object) array(
      'title' => '',
      'machine_name' => '',
      'sync_profile_name' => '',
      'mode' => 'A',
      'components' => array(),
    );
  }
  if (isset($form_state['values'])) {
    if (isset($form_state['values']['profile_object'])) {
      $profile = $form_state['values']['profile_object'];
    }
    $profile->sync_profile_name = $form_state['values']['sync_profile_name'];
  }
  if (!empty($form_state['args']['new_condition'])) {
    $filters = secsyn_filters_get_filters_full_list($form_state['args']['new_condition']['filter_profile']);
    $profile->components[] = (object) array(
      'local_order' => count($profile->components) + 1,
      'filter_key' => $form_state['args']['new_condition']['filter_type'],
      'filter_options' => $filters[$form_state['args']['new_condition']['filter_type']]['defaults'],
      'profile_name' => $form_state['args']['new_condition']['filter_profile'],
    );
  }
  if ((!empty($form_state['args']['_remove_element']['action'])) &&
      $form_state['args']['_remove_element']['action']) {
    $new_conditions = array();
    foreach ($profile->components as $component) {
      if ($component->local_order != $form_state['args']['_remove_element']['number']) {
        $new_conditions[] = $component;
      }
    }
    $profile->components = array();
    $k = 1;
    foreach ($new_conditions as $condition) {
      $condition->local_order = $k;
      $profile->components[] = $condition;
      $k++;
    }
  }

  /*
  $form_state['values']['_remove_element'] = array(
    'action' => TRUE,
    'number' => $rule_num,
  );*/
  $form = array(
    'filter_info' => array(
      '#type' => 'fieldset',
      '#title' => t('Filter Information'),
      '#collapsible' => TRUE,
      'title' => array(
        '#type' => 'textfield',
        '#title' => t('Filter Title'),
        '#required' => FALSE,
        '#maxlength' => 256,
        '#default_value' => $profile->title,
      ),
      'profile_object' => array(
        '#type' => 'value',
        '#value' => $profile,
      ),
    ),
  );
  if (empty($profile->machine_name)) {
    $form['filter_info']['machine_name'] = array(
      '#type' => 'machine_name',
      '#title' => t('Machine Name'),
      '#required' => FALSE,
      '#description' => t('Machine-friendly name'),
      '#maxlength' => 64,
      '#machine_name' => array(
        'exists' => '_secsyn_filters_profile_machine_name_check',
        'source' => array('filter_info', 'title'),
      ),
      '#default_value' => $profile->machine_name,
    );
  } else {
    $form['filter_info']['machine_name'] = array(
      '#type' => 'value',
      '#value' => $profile->machine_name,
    );
  }
  $profiles = secsyn_get_profiles();
  $profiles_with_filters = array();
  foreach ($profiles as $key => $value) {
    $syn_profile = SecureSyndicationProfile::getProfile($key);
    if (count($syn_profile->filters())) {
      $profiles_with_filters[$key] = $value;
    }
  }
  $form['filter_info']['sync_profile_name'] = array(
    '#type' => 'select',
    '#title' => t('Synchronization Profile'),
    '#options' => $profiles_with_filters,
    '#default_value' => '',
    '#empty_value' => '',
    '#empty_option' => t('Select one...'),
    '#required' => TRUE,
    '#description' => t('Syndication profile to use'),
    '#default_value' => $profile->sync_profile_name,
    '#ajax' => array(
      'event' => 'change',
      'callback' => 'secsyn_filters_filter_form_update',
        'wrapper' => 'secsyn_filters_block'
    ),
  );
  $form['filter_info']['mode'] = array(
    '#type' => 'value',
    '#value' => 'A',/*
    '#title' => t('Mode'),
    '#options' => array(
      'A' => 'Items that match all the filters',
      'O' => 'Items that match any of the filters',
    ),
    '#default_value' => $profile->mode,
    '#required' => TRUE,*/
  );

  $form['secsyn_filters_block'] = array(
    '#type' => 'fieldset',
    '#title' => t('Conditions'),
    '#tree' => TRUE,
    '#prefix' => '<div id="secsyn_filters_block">',
    '#suffix' => '</div>',
    'add_filter' => array(
      '#type' => 'container',
      '#title' => t('Add Condition'),
      '#collapsible' => TRUE,
    ),
    'filter_list' => array(
      '#type' => 'container',
      '#title' => t('Conditions'),
      '#collapsible' => TRUE,
    ),
  );

  $form['secsyn_filters_block']['add_filter'] += secsyn_filters_new_filter_control($profile->sync_profile_name);
  $form['secsyn_filters_block']['filter_list'] += secsyn_filters_existing_filters($form, $form_state, $profile);

  $form += array(
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Save Filter'),
      '#validate' => array('secsyn_filters_filter_form_validate'),
    )
  );
  return $form;
}
function secsyn_filters_filter_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] == 'Save Filter') {
    if (empty($form_state['values']['title'])) {
      form_set_error('title', t('You must specify a title'));
    }
    if (empty($form_state['values']['machine_name'])) {
      form_set_error('machine_name', t('You must specify a machine name'));
    }
    if ((!isset($form_state['values']['secsyn_filters_block']['filter_list'])) ||
        !(count($form_state['values']['secsyn_filters_block']['filter_list']))) {
      form_set_error('secsyn_filters_block', t('You must specify at least one condition'));
    }
    $profile = (object) array(
      'title' => $form_state['values']['title'],
      'machine_name' => $form_state['values']['machine_name'],
      'sync_profile_name' => $form_state['values']['sync_profile_name'],
      'mode' => $form_state['values']['mode'],
    );
    secsyn_filters_load_components_from_form($profile, $form_state);
    $form_state['values']['#profile_object'] = $profile;
  }
}

function secsyn_filters_remove_filter($form, &$form_state) {
  $parents = $form_state['triggering_element']['#parents'];
  $rule_name = $parents[count($parents) - 2];
  $rule_num = substr($rule_name, 5) - 1;
  $form_state['args']['_remove_element'] = array(
    'action' => TRUE,
    'number' => $rule_num,
  );
  $form_state['rebuild'] = TRUE;
}

function secsyn_filters_existing_filters($form, &$form_state, $profile) {
  $profile_rules = array();
  if (!empty($profile->sync_profile_name)) {
    $class_name = $profile->sync_profile_name;
    $instance = new $class_name();
    $filters = secsyn_filters_get_filters_full_list($profile->sync_profile_name);
    foreach ($profile->components as $component) {
      $entry = array(
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#title' => '#' . $component->local_order . ' - ' . t($filters[$component->filter_key]['title']),
        '#tree' => TRUE,
        '_base_component' => array(
          '#type' => 'value',
          '#value' => $component,
        ),
      );
      $entry += $instance->{$filters[$component->filter_key]['filter_form']}($component->filter_options);
      $entry['_remove_me'] = array(
        '#type' => 'checkbox',
        '#title' => t('Remove filter'),
      );
      $profile_rules['rule_' . $component->local_order] = $entry;
    }
  }
  return $profile_rules;
}

function secsyn_filters_filter_form_update($form, &$form_state) {
  return $form['secsyn_filters_block'];
}

function secsyn_filters_new_filter_control($profile_name) {
  if (empty($profile_name)) {
    return array(
      'new_filter' => array(
        '#type' => 'markup',
        '#markup' => '<p>' . t('You must select a profile before creating your filters'),
      ),
    );
  } else {
    return array(
      'new_filter' => array(
        '#title' => '',
        '#type' => 'select',
        '#options' => secsyn_filters_get_filter_list($profile_name),
        '#default_value' => '',
        '#empty_value' => '',
        '#empty_option' => 'Select one...',
      ),
      'new_filter_submit' => array(
        '#value' => t('Add Condition'),
        '#type' => 'submit',
        '#submit' => array('secsyn_filters_partial_submit'),
        '#ajax' => array(
          'callback' => 'secsyn_filters_filter_form_update',
          'wrapper' => 'secsyn_filters_block',
        )
      ),
    );
  }
}

function secsyn_filters_partial_submit($form, &$form_state) {
  $form_state['args']['new_condition'] = array(
    'filter_type' => $form_state['values']['secsyn_filters_block']['add_filter']['new_filter'],
    'filter_profile' => $form_state['values']['sync_profile_name'],
  );
  $form_state['rebuild'] = TRUE;
}

function secsyn_filters_get_filters_full_list($profile_type) {
  $profile = new $profile_type();
  return $profile->filters();
}

function secsyn_filters_get_filter_list($profile_type) {
  $filters = secsyn_filters_get_filters_full_list($profile_type);
  $list = array();
  foreach ($filters as $key => $filter) {
    $list[$key] = t($filter['title']);
  }
  return $list;
}

function secsyn_filters_load_components_from_form(&$profile, &$form_state) {
  $profile->components = array();
  $class = $form_state['values']['sync_profile_name'];
  $object = new $class();
  $filters = $object->filters();
  $components = array();
  foreach ($form_state['values']['secsyn_filters_block']['filter_list'] as $filter_element => $filter) {
    if (!$filter['_remove_me']) {
      $component = $filter['_base_component'];
      unset($filter['_base_component']);
      $component_value_method = $filters[$component->filter_key]['value_callback'];
      $callback = array(
        $object,
        $component_value_method,
      );
      $filter_options = call_user_func($callback, $filter);
      $callback = array(
        $object,
        $filters[$component->filter_key]['validate_callback'],
      );
      $element = 'secsyn_filters_block][filter_list][' . $filter_element;
      if (!call_user_func($callback, $element, $filter_options)) {
        return FALSE;
      }
      $component->filter_options = $filter_options;
      $components[$component->local_order] = $component;
    }
  }
  ksort($components);
  $k = 1;
  foreach ($components as $component) {
    $component->local_order = $k;
    $profile->components[] = $component;
    $k++;
  }
  return TRUE;
}

function secsyn_filters_filter_form_submit($form, &$form_state) {
  $profile = $form_state['values']['#profile_object'];
  secsyn_filters_profile_save($profile);
  drupal_set_message(t('Syndication filter saved'));
  $form_state['redirect'] = 'admin/content/syndication/filters';
}

function secsyn_filters_delete_filter_form_submit($form, &$form_state) {
  $profile = $form_state['values']['profile'];
  db_delete('secsyn_filters_profile')
      ->condition('machine_name', $profile->machine_name)
      ->execute();
  drupal_set_message(t('Filter deleted successfully'));
  $form_state['redirect'] = 'admin/content/syndication/filters';
}

function secsyn_filters_delete_filter_form($form, &$form_state, $profile) {
  $form['profile'] = array(
    '#value' => $profile,
    '#type' => 'value',
  );
  return confirm_form($form,
      t('Are you sure you want to delete this filter?'),
      'admin/content/syndication/filters',
      t('This action cannot be undone'),
      t('Delete')
  );
}