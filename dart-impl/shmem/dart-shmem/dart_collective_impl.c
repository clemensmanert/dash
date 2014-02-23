
#include "dart_types.h"
#include "dart_globmem.h"
#include "dart_communication.h"
#include "dart_team_group.h"
#include "shmem_p2p_if.h"
#include "shmem_logger.h"
#include "shmem_barriers_if.h"


dart_ret_t dart_barrier(dart_team_t teamid)
{
  if( teamid==DART_TEAM_ALL ) {
    shmem_syncarea_barrier_wait(0);
  } else {
    int slot;
    slot = shmem_syncarea_findteam(teamid);
    if( 0<=slot && slot<MAXNUM_TEAMS ) {
      shmem_syncarea_barrier_wait(slot);
    }
  }

  // todo: barrier on the other teams
  return DART_OK;
}


dart_ret_t dart_bcast(void *buf, size_t nbytes, 
		      dart_unit_t root, dart_team_t team)
{
  //SANITY_CHECK_TEAM(team);
  dart_unit_t myid;
  size_t size;

  int i;
  dart_team_myid(team, &myid);
  dart_team_size(team, &size);

  DEBUG("dart_bcast on team %d, root=%d, tsize=%d", team, root, size);
  if( myid==root ) 
    {
      for(i=0; i<size; i++) {
	if( i!=root ) {
	  DEBUG("dart_bcast sending to %d", i);
	  dart_shmem_send(buf, nbytes, team, i);
	}
      } 
  } 
  else 
    {
      dart_shmem_recv(buf, nbytes, team, root);
    }

  //shmem_barriers_barrier_wait(team);

  return DART_OK;
}

dart_ret_t dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, 
			dart_unit_t root, dart_team_t team)
{
  return DART_OK;
}

dart_ret_t dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, 
		       dart_unit_t root, dart_team_t team)
{
  return DART_OK;
}
  
dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, 
			  dart_team_t team)
{
  return DART_OK;
}