<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * Description of SecureSyndicationBaseUserProfile
 *
 * @author rturnbul
 */
class SecureSyndicationBaseUserProfile extends SecureSyndicationEntityProfile {

  public function lookup($uuid) {
    if (empty($uuid)) {
      return FALSE;
    }
    $pieces = explode('/', $uuid);
    $user = user_load_by_mail($pieces[2]);
    if ($user) {
      if ($user->uuid == $pieces[1]) {
        return $user;
      } else {
        $item = reset(entity_uuid_load('user', array($pieces[1])));
        if (empty($item)) {
          // there's no item with the UUID already, so let's just
          // give the existing item the correct UUID.
          db_update('users')
              ->fields(array(
                'uuid' => $pieces[1]
              ))
              ->condition('mail', $user->mail)
              ->execute();
          return user_load_by_mail($pieces[2]);
        } else {
          // we could actually handle this (maybe?) by
          // doing something like merging the user accounts.
          // but that's just messy
          watchdog('secsyn_entity', 'Conflicting user accounts found. Local: @local; Remote: @remote', array(
            '@local' => $user->uuid,
            '@remote' => $pieces[1],
          ), WATCHDOG_ERROR);
          throw new Exception('Conflicting user accounts found');
          return FALSE;
        }
      }
    } else {
      return reset(entity_uuid_load('user', array($pieces[1])));
    }
    return FALSE;
  }

  public function uuid($object) {
    if (empty($object->entity_type) || (!$object->entity_type == 'user')) {
      // we only handle users
      return FALSE;
    }
    if ($object->uid < 2) {
      // reject admin and anon users
      return FALSE;
    }
    // we need to include mail as well so we can avoid duplication of mail
    // on the other side
    return implode('/', array(
      'user',
      $object->uuid,
      $object->mail
    ));
  }

  public function filters() {
    return array();
  }

}

?>
