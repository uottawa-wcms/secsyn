<?php

function secsyn_menu_syndicate_form($form, &$form_state, $menu) {
  $form['menu'] = array(
    '#value' => $menu,
    '#type' => 'value',
  );
  _secsyn_common_fields($form);
  return $form;
}

function secsyn_menu_syndicate_form_submit($form, &$form_state) {
  _secsyn_fast_queue(
          $form_state['values']['remote_server'],
          'SecureSyndicationMenuProfile',
          $form_state['values']['menu'],
          $form_state['values']['urgency']);
  drupal_set_message(t('Menu has been queued for syndication'));
  $form_state['redirect'] = 'admin/structure/menu/manage/' . $form_state['values']['menu']['menu_name'];
}